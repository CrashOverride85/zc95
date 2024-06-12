import sys
import csv

class Pulse:
    def __init__(self, time_us, channel, pulse_width_pos, pulse_width_neg):
        self.time_us = time_us
        self.channel = channel
        self.pulse_width_pos = pulse_width_pos
        self.pulse_width_neg = pulse_width_neg

    def __str__(self):
        return "time_us: " + str(self.time_us) + ", channel: " + str(self.channel) + ", pulse_width_pos: " + str(self.pulse_width_pos) + ", pulse_width_neg: " + str(self.pulse_width_neg)


class csv_process:
    def __init__(self, file_path):
        self.file_path = file_path
        self.channel_pulses = {}
        self.channel_pulses[1] = []
        self.channel_pulses[2] = []
        self.channel_pulses[3] = []
        self.channel_pulses[4] = []

        self.pulses = []

    def sort_pulses(self, pulse_list):
        return sorted(pulse_list, key=lambda pulse: pulse.time_us)

    def process_entries_for_channel(self, channel, channel_pulses):
            for i in range(len(channel_pulses) - 1):
                next_timestamp_us = int(channel_pulses[i+1]['Timestamp']) - int(channel_pulses[i]['Timestamp'])
                if (next_timestamp_us < 2000):
                    if next_timestamp_us > 255:
                        # The ZC95 can only generate a positive pulse followed immediately by a negative pulse (although either can be 0us).
                        # It can't generate a positive pulse, wait e.g. 400us, then generate the negative pulse. Or do the negative going pulse
                        # first. Once a positive/negative pulse pair has been generated, there should be delay of _at least_ 2000us (so 500hz)
                        # before the next pair, but ideally more like a delay of 4000us (250hz) or more.
                        print("ERROR - pulse 256-2000us after previous (seqnum: ) " + str(channel_pulses[i]['SeqNr']))
                        continue

                    pulse = Pulse(
                        int(channel_pulses[i]['Timestamp']),
                        channel,
                        int(channel_pulses[i]['Width']),
                        int(channel_pulses[i+1]['Width'])
                    )
                    self.pulses.append(pulse)
                    i += 1

                elif channel_pulses[i]['Phase'] == channel_pulses[i+1]['Phase']:         # same phase, and next_timestamp_us >= 2000

                    if int(channel_pulses[i]['Phase']) == 0:
                        pulse_width_pos = int(channel_pulses[i]['Width'])
                        pulse_width_neg = 0
                    elif int(channel_pulses[i]['Phase']) == 1:
                        pulse_width_pos = 0
                        pulse_width_neg = int(channel_pulses[i]['Width'])
                    else:
                        print("ERROR - unexpected phase (seqnum: ) " + str(channel_pulses[i]['SeqNr']))
                        continue

                    pulse = Pulse(
                        int(channel_pulses[i]['Timestamp']),
                        1, # channel
                        pulse_width_pos,
                        pulse_width_neg
                    )
                    self.pulses.append(pulse)

    def parse_csv(self):
        with open(self.file_path, mode='r', newline='') as csvfile:
            reader = csv.DictReader(csvfile)

            first_row = next(reader)
            time_offset = int(first_row['Timestamp'])
            first_row['Timestamp'] = 0

            self.channel_pulses[1].append(first_row)

            # Split channels and rebase timestamps
            for row in reader:
                row['Timestamp'] = int(row['Timestamp']) - time_offset
                if row['Stage'] == "A":
                    self.channel_pulses[1].append(row)
                elif row['Stage'] == "B":
                    self.channel_pulses[2].append(row)

            # Covert data in csv for each channel into something the zc95 can use
            self.process_entries_for_channel(1, self.channel_pulses[1])
            self.process_entries_for_channel(2, self.channel_pulses[2])

            # Recombine channels into a single pulse stream
            return self.sort_pulses(self.pulses)

    def write_pulses_to_csv(self, pulse_list, filename):
        with open(filename, 'w', newline='') as file:
            writer = csv.writer(file)

            # Write the header
            writer.writerow(['time_us', 'channel', 'pos_width_us', 'neg_width_us', 'power'])

            # Write the pulse data
            for pulse in pulse_list:
                writer.writerow([pulse.time_us, pulse.channel, pulse.pulse_width_pos, pulse.pulse_width_neg, 1000])


if len(sys.argv) != 2:
    print("Error - must specify csv to convert")
    exit()

file_path = sys.argv[1]
print("Reading from: " + file_path)
csvp = csv_process(file_path)

zc95_pulses = csvp.parse_csv()
output_file = file_path.replace(".csv", ".zc95.csv")
print("Writing to: " + output_file)
if (output_file == file_path):
    print("Error: failed to generate output filename")
    exit()

csvp.write_pulses_to_csv(zc95_pulses, output_file)
