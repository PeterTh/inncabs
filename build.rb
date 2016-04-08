require 'color.rb'

COMPILER = "/software-local/insieme-libs/with_gcc_5.2.0/gcc-5.2.0/bin/g++"
CPPFLAGS = "-std=c++14 -Wall -L /software-local/insieme-libs/with_gcc_5.2.0/gcc-5.2.0/lib64 -lpthread -I lib/parec/code/include -fmax-errors=2 -fcilkplus"

debug = ARGV.include?("--dbg")
parallel = ARGV.include?("--par")
clean = ARGV.include?("--clean")
ARGV.delete_if { |e| e.start_with?("--") }

threads = []

Dir["**/*.cpp"].each do |cppfile|
	next if !ARGV.empty? && !ARGV.any? { |arg| cppfile =~ /#{arg}/ }
	job = lambda do
		fname = File.basename(cppfile, ".cpp")
		outfname =  "bin/" + fname
		if(clean) then
			File.delete(outfname) if File.exists?(outfname)
			puts "======== Cleaned " + fname.red.bold
			return
		end
		command = "#{COMPILER} #{cppfile} -o #{outfname} #{CPPFLAGS}"
		#command = "/software-local/insieme-libs/gcc-4.9.0/bin/g++ #{cppfile} -o #{outfname} -std=c++11 -Wall -lpthread"
		command += " -g -DINNCABS_DEBUG  -DINNCABS_MSG" if debug
		command += " -O3" if !debug
		puts "======== Building " + fname.green.bold + (debug && " debug" || " release") + " version"
		`#{command}`
	end
	if(parallel) then threads << Thread.new { job.call }
	else job.call end
end

threads.each { |t| t.join }

