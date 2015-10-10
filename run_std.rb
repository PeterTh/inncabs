#!/usr/bin/env ruby

#specify the directory for repo of inncabs 
#     (uses *.cpp filenames or you may specify the executables to run)
repo_dir = '../'

#specify build directory for inncabs
build_dir = './'
#specify the apps to run
apps = ["alignment","fft", "fib", "floorplan", "health", "intersim", 
        "nqueens", "pyramids", "qap", "round", "sort", "sparselu", 
        "strassen", "uts"]

#query the number of cores on this threads per core, sockets, sockets
nthreads_per_core = `lscpu|grep "Thread(s) per core"`.gsub(/[^\d]/,"").to_i
ncores_per_socket = `lscpu|grep "Core(s) per socket"`.gsub(/[^\d]/,"").to_i
nsockets = `lscpu|grep "Socket(s)"`.gsub(/[^\d]/,"").to_i
ncores = ncores_per_socket * nsockets
nproc = ncores * nthreads_per_core

#using clang or g++ 
clang=ARGV.include?("--clang")
gpp=ARGV.include?("--g++")
if (clang) then
    idx = ARGV.index("--clang")
    ARGV.delete_at(idx)
    build_dir = build_dir + 'clang_build/'
end
if (gpp) then
    idx = ARGV.index("--g++")
    ARGV.delete_at(idx)
    build_dir = build_dir + 'g++_build/'
end


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

cores = [1,2,4,6,8,10,12,14,16,18,20]
#launch_types = %w(deferred optional async)
launch_types = %w(async)

repeats = read_int_param("--repeats", 5)
timeout_secs = read_int_param("--timeout", 100)
#read threads per core to run on
th_per_core = read_int_param("--threads_per_core", 1)

params = {
    "alignment" => repo_dir + "bin/input/alignment/prot.100.aa",
    "fft" => "1000000",
    "fib" => "30",
    "floorplan" => repo_dir + "bin/input/floorplan/input.15",
    "health" => repo_dir + "bin/input/health/medium.input",
    "intersim" => "150",
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

print "\nBenchmarks Path: ", build_dir,"bin\n"
print "Cores = ", cores, " running on ", th_per_core, " thread(s) per core \n"
print "Repeats (number of samples) = ", repeats,"\n"
print "Launch Types = ",launch_types, "\n"
print "Timeout set to ", timeout_secs, " seconds.\n\n"
print "Benchmarks:", apps, "\n\n"

apps.each do |cppfile|
    next if !ARGV.empty? && !ARGV.any? { |arg| cppfile =~ /#{arg}/ }
    launch_types.each do |launch_type|
        cores.each do |num_cpus|
            if(launch_type == "deferred" && num_cpus > cores[0])
                next
            end
            fname = File.basename(cppfile, ".cpp")
            binfname =  build_dir + "bin/" + fname

            # th_per_core is the number of threads run on a core
            # num_cpus is the number of cores to run on
            
            # the following needs to be modified for more than 2 sockets
            if (num_cpus <= ncores_per_socket)  
                cpulist = (0..num_cpus-1).map{|a| a*2}
            else 
                cpulist = (0..ncores_per_socket-1).map{|a| a*2}\
                    ::concat((1..num_cpus-ncores_per_socket).map{|a| a*2-1})
            end
            if (th_per_core == 2)
                cpulist = (cpulist::concat(cpulist.map{|a| a+20}))
            end
            cpulist=cpulist.to_a.join(",")
            
            if(OS.windows?)
				command  = "set INNCABS_REPEATS=#{repeats}\nset INNCABS_MIN_OUTPUT=true\n"
				command += "set INNCABS_LAUNCH_TYPES=#{launch_type}\nset INNCABS_TIMEOUT=#{timeout_secs*1000}\n"
                command += "start #{win_cpu_aff[num_cpus]} /b /wait #{binfname}"
            else
                command = "timeout #{timeout_secs} taskset -c #{cpulist} #{binfname}"
				command = "export INNCABS_REPEATS=#{repeats}\n" + command
                command = "export INNCABS_MIN_OUTPUT=true\n" + command
				command = "export INNCABS_TIMES_OUTPUT=true\n" + command
                command = "export INNCABS_LAUNCH_TYPES=#{launch_type}\n" + command
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
