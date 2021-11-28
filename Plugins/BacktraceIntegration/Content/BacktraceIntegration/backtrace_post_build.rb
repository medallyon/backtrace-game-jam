require 'date'
require 'digest/md5'
require 'fileutils'
require 'inifile'
require 'json'
require 'optparse'
require 'pathname'
require 'rubygems'
require 'set'
require 'zip'

exit if defined? Ocra

opts = {
    send_debug: false,
    send_release: true,
    upload_editor: true,
    cmd: ['ruby', $0, ARGV].flatten.join(' '),
}

def log(s)
    puts "[BacktracePostBuild] #{s}"
end

OptionParser.new do |o|
    o.on('--token=VALUE') do |v| opts[:token] = v end
    o.on('--symbols_token=VALUE') do |v| opts[:symbols_token] = v end
    o.on('--send_debug=VALUE') do |v| opts[:send_debug] = (v.downcase == 'true' ? true : false) end
    o.on('--send_release=VALUE') do |v| opts[:send_release] = (v.downcase == 'true' ? true : false) end
    o.on('--realm=VALUE') do |v| opts[:realm] = v end
    o.on('--engine_dir=VALUE') do |v| opts[:engine_dir] = v.gsub('\\', '/') end
    o.on('--project_dir=VALUE') do |v| opts[:project_dir] = v.gsub('\\', '/') end
    o.on('--target_name=VALUE') do |v| opts[:target_name] = v end
    o.on('--target_configuration=VALUE') do |v| opts[:target_configuration] = v end
    o.on('--target_type=VALUE') do |v| opts[:target_type] = v end
    o.on('--project_file=VALUE') do |v| opts[:project_file] = v end
    o.on('--upload_editor=VALUE') do |v| opts[:upload_editor] = (v.downcase == 'true' ? true : false) end
end.parse!

IS_DEBUG = opts[:target_type] != 'Game'

opts.each do |k, v|
    log "Option: #{k} = #{v}"
end

log 'Debug build: %s' % IS_DEBUG

if IS_DEBUG and not opts[:send_debug]
    log 'Not instructed to send pdbs for debug build, exiting...'
    exit
end

ZIP_FILE = "#{opts[:project_dir]}/symbols.zip".gsub('/','\\')
log "Zip file: #{ZIP_FILE}"

CONFIG_FILE = '%{project_dir}/Plugins/BacktraceIntegration/backtrace_state.json' % opts
json_data = '{}'
begin
    json_data = File.read CONFIG_FILE
rescue
end

cfg = JSON.parse json_data

staged = Dir['%{project_dir}/Saved/StagedBuilds/*' % opts]
    .map{ |p| Pathname.new p }
    .select{ |p| p.directory? }
    .map{ |p| p.each_filename.to_a.last }

staged = staged + ['WindowsNoEditor']
staged.uniq!

if staged.size > 1 or (staged.size == 1 and staged[0] != 'WindowsNoEditor')
    log "Warning: unexpected directories found: %s" % staged.join(", ")
end

log "Staged dirs: #{staged.join(', ')}"

CRC_PATH = "%{engine_dir}/Binaries/Win64/CrashReportClient.exe" % opts

def add_or_update_router_in_ini(filename, opts)
    log "ini update for #{filename}"
    ini = IniFile.load(filename)
    ini = IniFile.new unless ini

    section = ini['CrashReportClient']
    router = section.fetch('DataRouterUrl', 'https://datarouter.ol.epicgames.com')

    if router !~ /^https:\/\/unreal\..backtrace\.io/i
        new_router = "https://unreal.backtrace.io/post/%{realm}/%{token}" % opts
        log "Replacing router value of #{router} with #{new_router}"
        section['DataRouterUrl'] = new_router
        ini['CrashReportClient'] = section
        ini.save(filename: filename)
    end
end

if not File.exists? CRC_PATH
    log "Error: cannot find CrashReportClient.exe in #{CRC_PATH}!"
else
    staged.each do |dir|
        destdir = "%{project_dir}/Saved/StagedBuilds/#{dir}/Engine/Binaries/Win64" % opts
        srcdir = "%{engine_dir}/Binaries/Win64/" % opts
        FileUtils.mkdir_p destdir
        FileUtils.cp "#{srcdir}/CrashReportClient.exe", "#{destdir}/CrashReportClient.exe"

        
        inipath = "%{project_dir}/Saved/StagedBuilds/#{dir}/Engine/Programs/CrashReportClient/Config" % opts
        inipath_glob = "%{project_dir}/Saved/StagedBuilds/#{dir}/Engine/Config" % opts

        FileUtils.mkdir_p inipath
        FileUtils.mkdir_p inipath_glob

        add_or_update_router_in_ini "#{inipath}/DefaultEngine.ini", opts
        add_or_update_router_in_ini "#{inipath}/UserEngine.ini", opts
        add_or_update_router_in_ini "#{inipath}/BaseEngine.ini", opts
        add_or_update_router_in_ini "#{inipath_glob}/DefaultEngine.ini", opts
        add_or_update_router_in_ini "#{inipath_glob}/UserEngine.ini", opts
        add_or_update_router_in_ini "#{inipath_glob}/BaseEngine.ini", opts
        add_or_update_router_in_ini "#{inipath_glob}/Base.ini", opts
    end
end

if opts[:upload_editor]
    begin
        already_done = cfg.fetch('already_done', []).to_set

        engine_dir = '%{engine_dir}' % opts

        Dir.chdir(engine_dir) do
            all_lib_files = Dir['**/*'].select{ |s| s =~ /(?:pdb|dll)$/ }
            editor_dlls = all_lib_files.select{ |s| s =~ /Binaries\/Win64\/UE4Editor-\w+\.(?:pdb|dll)$/ }.to_a
            global_plugins = all_lib_files.select{ |s| s =~ /^Plugins\/Runtime/ }.to_a

            files = global_plugins

            zipped = 0

            FileUtils.rm_f ZIP_FILE
            Zip::File.open(ZIP_FILE, Zip::File::CREATE) do |zipfile|
                files.sort.uniq.each do |filename|

                    digest = Digest::MD5.hexdigest(File.binread filename)
                    if not already_done.member? digest
                        zipfile.add filename, File.join(engine_dir, filename)
                        already_done.add digest
                        zipped += 1
                    end
                end
            end

            cmd = "\"%{project_dir}/Plugins/BacktraceIntegration/Content/BacktraceIntegration/uploader.exe\" %{realm} %{symbols_token} \"#{ZIP_FILE}\"" % opts
            
            if zipped > 0 and system cmd
                cfg['already_done'] = already_done.to_a
            end
            
            FileUtils.rm_f ZIP_FILE

            File.write CONFIG_FILE, JSON.pretty_generate(cfg)
        end
    rescue => e
        log "Exception when uploding editor symbols: #{e}"
    end
end

Dir.chdir(opts[:project_dir]) do
    files = Dir['**/*']
        .reject{ |f| f =~ /(?:ModuleRules)(?:\.exe|\.pdb|\.dll)$/ }
        .reject{ |f| f =~ /^Plugins\/BacktraceIntegration/ }
        .reject{ |f| f =~ /^Saved\/StagedBuilds\/WindowsNoEditor\// }
        .select{ |f| f =~ /\.(?:exe|pdb|dll)$/ }
        

    FileUtils.rm_f ZIP_FILE
    Zip::File.open(ZIP_FILE, Zip::File::CREATE) do |zipfile|
        files.each do |filename|
            zipfile.add filename, File.join(opts[:project_dir], filename)
        end
    end

    cmd = "\"%{project_dir}/Plugins/BacktraceIntegration/Content/BacktraceIntegration/uploader.exe\" %{realm} %{symbols_token} \"#{ZIP_FILE}\"" % opts
    puts cmd
    system cmd
    
    # FileUtils.rm_f ZIP_FILE
end
