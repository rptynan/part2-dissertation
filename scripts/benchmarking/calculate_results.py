#!/usr/bin/python
import sys
import re
import math

stats = [
    'hit_static_case',
    'hit_heap_case',
    'aborted_static',
    'aborted_unindexed_heap',
    'aborted_unknown_storage',
    'succeeded',
    'failed',
    'aborted_stack',
]
directory = str(sys.argv[1])
num_runs = int(sys.argv[2])


class Benchmark:
    def __init__(self, name, unit, metric_regex):
        self.name = name
        self.unit = unit
        self.metric_regex = metric_regex
        self.instances = []
        return

    def do_stddev(self, ls):
        mean = self.do_mean(ls)
        return math.sqrt(
            sum([(r-mean) * (r-mean) for r in ls]) / len(ls)
        )

    def do_mean(self, ls):
        return sum(ls) / len(ls)


    def get_results(self):
        if self.unit == 'ops/s':
            # Take reciprocal for better is higher metrics
            return [1 / float(re.search(self.metric_regex, '\n'.join(ins)).group(1))
                    for ins in self.instances]
        else:
            return [float(re.search(self.metric_regex, '\n'.join(ins)).group(1))
                    for ins in self.instances]

    def get_results_mean(self):
        return self.do_mean(self.get_results())

    def get_results_stddev(self):
        return self.do_stddev(self.get_results());


    def is_crunched(self):
        return 'debug.libcrunch' in '\n'.join(self.instances[0])

    def get_stats(self):
        res = {}
        for stat in stats:
            res[stat] = [
                float(re.search(stat + ':\s(\d+)', '\n'.join(ins[-30:-1])).group(1))
                for ins in self.instances
            ]
        return res

    def get_calc_stats(self):
        sts = self.get_stats()
        return {
         'unanswered_impl':
            [(sts['aborted_static'][i] + sts['aborted_unindexed_heap'][i])
             / (sts['hit_static_case'][i] + sts['hit_heap_case'][i])
             for i in range(0, num_runs)],
         'unanswered_total':
            [sts['aborted_unknown_storage'][i]
             / (sts['hit_static_case'][i] + sts['hit_heap_case'][i]
                 + sts['aborted_unknown_storage'][i])
             for i in range(0, num_runs)],
         'failure_rate':
            [sts['failed'][i] / (sts['failed'][i] + sts['succeeded'][i])
             for i in range(0, num_runs)],
          'stack_fraction':
            [sts['aborted_stack'][i] / sts['aborted_unknown_storage'][i]
             for i in range(0, num_runs)],
        }

    def get_calc_stats_mean(self):
        sts = self.get_calc_stats()
        return {k: self.do_mean(sts[k]) for k in sts}

    def get_calc_stats_stddev(self):
        sts = self.get_calc_stats()
        return {k: self.do_stddev(sts[k]) for k in sts}


tests = ['cputest', 'filetest', 'memorytest', 'mutextest', 'threadtest']
benchmarks = {
    'cputest' : Benchmark('cputest', 's', '\s*total time:\s*([\d\.]+)s'),
    'filetest' : Benchmark('filetest', 'ops/s', '\s*([\d\.]+)\s*Requests/sec'),
    'memorytest' : Benchmark('memorytest', 'ops/s', '\(([\d\.]+)\s*ops/sec\)'),
    'mutextest' : Benchmark('mutextest', 's', '\s*total time:\s*([\d\.]+)s'),
    'threadstest' : Benchmark('threadstest', 'ms', '\s*avg:\s*([\d\.]+)ms'),
}

for name in benchmarks:
    benchmark = benchmarks[name]
    for i in range(1, num_runs + 1):
        with open(directory + '/' + name + '/run' + str(i)) as f:
            benchmark.instances.append(f.readlines())
    print name, benchmark.unit
    print benchmark.get_results()
    print benchmark.get_results_mean()
    print benchmark.get_results_stddev()
    if benchmark.is_crunched():
        print benchmark.get_stats()
        print benchmark.get_calc_stats()
        print benchmark.get_calc_stats_mean()
        print benchmark.get_calc_stats_stddev()
    print('')


