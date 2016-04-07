

#res_names = %w(clang clang_limit64 gcc gcc_limit64 win64)
res_names = %w(async)
$stddev = 1

launch_types = %w(async)
benchmarks = %w(alignment fib floorplan health nqueens qap sort sparselu strassen pyramids uts)
min_cpus = 1
max_cpus = 64

$max_result = 900000
$width = 24

def read_result(results, benchname, launchtype, numcpus, num)
	numcpus = 1 if launchtype == "deferred"
	return $max_result unless results.include?(benchname)
	benchresults = results[benchname]
	return $max_result unless benchresults.include?(launchtype)
	launchresults = benchresults[launchtype]
	return $max_result unless launchresults.include?(numcpus)
	result = launchresults[numcpus][num]
	return $max_result if result == 0 
	return result
end

benchmarks.each do |benchname|
	# Benchmark name
	puts "\n" + benchname
	# Headers
	print "#Cores ; ".rjust(8)
	res_names.each do |resname|
		print "#{resname} ms ; ".rjust($width)
		print "#{resname} stddev ; ".rjust($width)
	end
	puts
	# Content
	i = min_cpus
	while(i <= max_cpus)
		print "#{i} ; ".rjust(8)
		res_names.each do |resname|
			results = eval(IO.read("results/results_" + resname + ".rb"))
			launch_types.each do |launchtype|
				print "#{read_result(results, benchname, launchtype, i, 0)} ; ".rjust($width)
				print "#{read_result(results, benchname, launchtype, i, $stddev)} ; ".rjust($width)
			end
		end
		puts
		i *= 2
	end
end
	