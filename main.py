import serial
import numpy

pico = serial.Serial("COM11", 115200)

"""
Classify based on timings.
1. calculate the standard deviation.
2. calculate the mean.
3. retreive the timings

Implement a score based system for inactivity.
"""

read_list = []

signals = [{'class': '', 'pullup': False, 'std': [], 'mean': [], 'no_time': []} for i in range(8)]
signal_timings = [[] for i in range(8)]


for i in range(800):
    ind = int(pico.readline().decode('utf-8').strip('\r\n'))
    read_list.append([ind, pico.readline()])
    print(i, ind)

# print(read_list)

mean = 0
for ind, i in read_list:
    real_ind = ind
    data = i.decode('utf-8')[:-2]
    one_counts = str(data).count('1')
    zero_counts = len(data) - one_counts
    if data != '\r\n':
        high_timings = []
        low_timings = []
        timings = []
        count = 0
        pre = data[0]
        first = True
        for j in data:
            if j != pre:
                if first:
                    first = False
                else:
                    if pre == '0':
                        timings.append(count)
                        low_timings.append(count)
                    elif pre == '1':
                        timings.append(count)
                        high_timings.append(count)
                count = 0
            pre = j
            count += 1

        low_timings = numpy.array(low_timings)
        high_timings = numpy.array(high_timings)

        low_std = numpy.std(low_timings)
        high_std = numpy.std(high_timings)
        low_mean = numpy.mean(low_timings)
        high_mean = numpy.mean(high_timings)
        
        print(i)
        print(low_timings)
        print(high_timings)
        print(timings)

        if len(timings) == 0:
            continue

        print('LOW timings.')
        print('std: ', low_std)
        print('mean: ', low_mean)
        print('HIGH timings.')
        print('std: ', high_std)
        print('mean: ', high_mean)

        if (((one_counts < zero_counts) and high_std < 1) or ((one_counts > zero_counts) and low_std < 1)):
            signals[real_ind]['class'] = 'clk'
        else:
            signals[real_ind]['class'] = 'data'

        if (one_counts < zero_counts):
            signals[real_ind]['std'].append(float(high_std))
            signals[real_ind]['mean'].append(float(high_mean))
            signals[real_ind]['no_time'].append(len(timings))
            signals[real_ind]['pullup'] = False
        else:
            signals[real_ind]['std'].append(float(low_std))
            signals[real_ind]['mean'].append(float(low_mean))
            signals[real_ind]['no_time'].append(len(timings))
            signals[real_ind]['pullup'] = True
        signal_timings[real_ind].append(timings)
        


print(*signals)
print('\n\n\n')
print(*signal_timings)
print('\n\n\n')
print(signal_timings[3])

