#!/usr/bin/env ruby

#specify the directory for repo of inncabs
repo_dir = './'

#specify build directory for inncabs
build_dir = './'

require repo_dir + 'os.rb'
require repo_dir + 'color.rb'

def read_int_param(name, default)
    param = default
    if(ARGV.include?(name))
        idx = ARGV.index(name)
        param = ARGV[idx+1].to_i
        ARGV.delete_at(idx)
        ARGV.delete_at(idx)
    end
    return param
end

# set the core counts, apps & launch type to run 

cores = [1,2,4,8,16,20,32,40]
apps = ["alignment", "fft", "fib", "floorplan", "health", "intersim", 
        "nqueens", "pyramids", "qap", "round", "sort", "sparselu", 
        "strassen", "uts"]
launch_types = %w(deferred optional async)

print "Preparing to run: " 
apps.each do |cppfile|
    print cppfile.green.bold , " "
end
    print "\n"

repeats = read_int_param("--repeats", 5)
timeout_secs = read_int_param("--timeout", 100)

#query the number of cores on this node and threads per core
nproc = `cat /proc/cpuinfo | grep processor | wc -l`.to_i
th_per_core= `lscpu|grep "Thread(s) per core"`.gsub(/[^\d]/,"")

params = {
    "alignment" => repo_dir + "bin/input/alignment/prot.100.aa",
    "fft" => "1000000",
    "fib" => "30",
    "floorplan" => repo_dir + "bin/input/floorplan/input.15",
    "health" => repo_dir + "bin/input/health/medium.input",
    "intersim" => "50",
    "nqueens" => "13",
    "pyramids" => "",
    "qap" => repo_dir + "bin/input/qap/chr12c.dat",
    "round" => "512 10",
    "sort" => "100000000 8192 2048 128",
    "sparselu" => "",
    "strassen" => "4096",
    "uts" => repo_dir + "bin/input/uts/tiny.input"
}

win_cpu_aff = {
    1 => "/AFFINITY 0x1",
    2 => "/AFFINITY 0x5",
    4 => "/AFFINITY 0x55",
    8 => "/AFFINITY 0x5555",
    16 => "/AFFINITY 0x55555555",
    32 => "/AFFINITY 0x5555555555555555",
    64 => "/AFFINITY 0xFFFFFFFFFFFFFFFF"
}

results = Hash.new { |h,k| h[k] = Hash.new { |h,k| h[k] = Hash.new { |h,k| h[k] = Array.new(2) } } } 

apps.each do |cppfile|
    next if !ARGV.empty? && !ARGV.any? { |arg| cppfile =~ /#{arg}/ }
    launch_types.each do |launch_type|
        cores.each do |num_cpus|
            if(launch_type == "deferred" && num_cpus > cores[0])
                next
            end
            fname = File.basename(cppfile, ".cpp")
            binfname =  build_dir + "bin/" + fname
    
            if th_per_core == "1"
                cpulist = (0..num_cpus-1).to_a.join(",")
            else
                cpulist = (0..nproc-1).step(2).to_a.concat((1..nproc-1).step(2).to_a)[0..num_cpus-1].join(",")
            end
            #print "cpulist ", cpulist, "\n"
                  
            if(OS.windows?)
				command  = "set INNCABS_REPEATS=#{repeats}\nset INNCABS_MIN_OUTPUT=true\n"
				command += "set INNCABS_LAUNCH_TYPES=#{launch_type}\nset INNCABS_TIMEOUT=#{timeout_secs*1000}\n"
                command += "start #{win_cpu_aff[num_cpus]} /b /wait #{binfname}"
            else
                command = "timeout #{timeout_secs} taskset -c #{cpulist} #{binfname}"
				command = "export INNCABS_REPEATS=#{repeats}\nexport INNCABS_MIN_OUTPUT=true\nexport INNCABS_LAUNCH_TYPES=#{launch_type}\n" + command
                command = "ulimit -t #{timeout_secs*num_cpus}\n" + command
            end
            command += " " + params[fname] if params.include?(fname)
            print "======== running " + fname.green.bold + " (#{launch_type}, #{num_cpus}): " 
            #puts command
            if(OS.windows?)
                File.open("run.bat", "w+") { |f| f.puts command }
                command = "run.bat"
            end
                result = `#{command}`
            if(result =~ /(\d+(?:\.\d+)?),(\d+(?:\.\d+)?)/)
                puts result.strip.bold
                results[fname][launch_type][num_cpus][0] = $1.to_f
                results[fname][launch_type][num_cpus][1] = $2.to_f
            else
                puts "timeout".bold
                results[fname][launch_type][num_cpus][0] = 0
                results[fname][launch_type][num_cpus][1] = 0
            end
            File.open("results.rb","w+") do |f|
                f.puts "results = " + results.inspect
            end
        end
    end
end
