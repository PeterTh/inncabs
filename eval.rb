#!/usr/bin/env ruby


#res_names = %w(clang clang_limit64 gcc gcc_limit64 win64)
res_names = %w(clang gcc win64)
res_names = %w(clang gcc)
$time = 0
$stddev = 1

launch_types = %w(deferred optional async)
benchmarks = %w(alignment fft fib floorplan health intersim nqueens pyramids qap round sort sparselu strassen uts)
cores = [1,2,4,8,16,20,32,40]

$max_result = 900000
$width = 24

def read_result(results, benchname, launchtype, numcpus)
	numcpus = 1 if launchtype == "deferred"
	return $max_result unless results.include?(benchname)
	benchresults = results[benchname]
	return $max_result unless benchresults.include?(launchtype)
	launchresults = benchresults[launchtype]
	return $max_result unless launchresults.include?(numcpus)
	#result = launchresults[numcpus][$stddev]
	result = launchresults[numcpus][$time]
	return $max_result if result == 0 
	return result
end

benchmarks.each do |benchname|
	# Benchmark name
	puts "\n" + benchname
	# Headers
	print "#Cores , ".rjust(8)
	res_names.each do |resname|
		launch_types.each do |launchtype|
			print "#{resname}:#{launchtype} , ".rjust($width)
		end
	end
	puts
	# Content
    cores.each do |i|
		print "#{i} , ".rjust(8)
		res_names.each do |resname|
			results = eval(IO.read("results/results_" + resname + ".rb"))
            launch_types.each do |launchtype|
				print "#{read_result(results, benchname, launchtype, i)} , ".rjust($width)
			end
		end
		puts
	end
end
	
