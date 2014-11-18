require './color.rb'
require './os.rb'

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

min_cpus = read_int_param("--min-cpus", 1)
max_cpus = read_int_param("--max-cpus", 8)
repeats = read_int_param("--repeats", 5)
timeout_secs = read_int_param("--timeout", 100)

launch_types = %w(deferred optional async)

params = {
	"alignment" => "bin/input/alignment/prot.100.aa",
	"fft" => "1000000",
	"fib" => "30",
	"floorplan" => "bin/input/floorplan/input.15",
	"health" => "bin/input/health/medium.input",
	"intersim" => "50",
	"nqueens" => "13",
	"pyramids" => "",
	"qap" => "bin/input/qap/chr12c.dat",
	"round" => "512 10",
	"sort" => "100000000 8192 2048 128",
	"sparselu" => "",
	"strassen" => "4096",
	"uts" => "bin/input/uts/tiny.input"
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

Dir["**/*.cpp"].each do |cppfile|
	next if !ARGV.empty? && !ARGV.any? { |arg| cppfile =~ /#{arg}/ }
	num_cpus = min_cpus
	launch_types.each do |launch_type|
		num_cpus = min_cpus
		while(num_cpus <= max_cpus)
			if(launch_type == "deferred" && num_cpus > min_cpus)
				num_cpus *= 2
				next
			end
			fname = File.basename(cppfile, ".cpp")
			binfname =  "bin/" + fname
			cpulist = (0..num_cpus-1).to_a.join(",")
			if(OS.windows?)
				command  = "set INNCABS_REPEATS=#{repeats}\nset INNCABS_MIN_OUTPUT=true\n"
				command += "set INNCABS_LAUNCH_TYPES=#{launch_type}\nset INNCABS_TIMEOUT=#{timeout_secs*1000}\n"
				command += "start #{win_cpu_aff[num_cpus]} /B /WAIT #{binfname}"
			else
				command = "timeout #{timeout_secs} taskset -c #{cpulist} #{binfname}"
				command = "export INNCABS_REPEATS=#{repeats}\nexport INNCABS_MIN_OUTPUT=true\nexport INNCABS_LAUNCH_TYPES=#{launch_type}\n" + command
				command = "ulimit -t #{timeout_secs*num_cpus}\n" + command
			end
			command += " " + params[fname] if params.include?(fname)
			print "======== Running " + fname.green.bold + " (#{launch_type}, #{num_cpus}): " 
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
			num_cpus *= 2
		end
	end
end
