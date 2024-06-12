import asyncio
import time
import struct
import queue
import csv
import sys
from bleak import BleakClient
from lib.ZcBle import ZcBle, ZcBlePulseMessage

# Read a CSV file with these fields:
#   time_us,channel,pos_width_us,neg_width_us,power
#
# Scans for ZC95, connects, then starts streaming the pulses from the CSV to it.
# When all have been sent, it goes back to the start.

async def main(address):

    def pattern_time_us(start_time_us):
        return ((time.time_ns() / 1000) - start_time_us)

    def exit_if_field_not_present(fieldnames, field):
        if field not in fieldnames:
            print("[" + field + "] field is missing from csv")
            exit()

    def is_int_in_string(string):
        try:
            int(string)
            return True
        except (ValueError, TypeError):
            return False

    def get_error_string(line_number, row, field):
        try:
            value = str(row[field])
        except:
            value = ""

        return "error on line " + str(line_number) + ": [" + value + "] is not valid for " + field

    def load_pulses_from_csv(filename):
        pulses = []

        with open(csv_filename, mode='r', newline='') as csvfile:
            reader = csv.DictReader(csvfile)

            exit_if_field_not_present(reader.fieldnames, "time_us")
            exit_if_field_not_present(reader.fieldnames, "channel")
            exit_if_field_not_present(reader.fieldnames, "pos_width_us")
            exit_if_field_not_present(reader.fieldnames, "neg_width_us")
            exit_if_field_not_present(reader.fieldnames, "power")

            line = 2 # line 1 = header, line 2 first data row. Yes, we're reporting line numbers starting at 1 not 0 as that's what most editors show
            last_time_us = 0
            for row in reader:
                # Vaidate the rows makes sense
                #####

                # correct number of fields
                # if len(row) != 5:
                #     sys.exit("error on line " + str(line_number) + ": expected 5 fields, found " + str(len(row)))

                # time_us
                if not is_int_in_string(row['time_us']):
                    sys.exit(get_error_string(line, row, 'time_us') + " - must integer")

                if int(row['time_us']) < last_time_us:
                    sys.exit(get_error_string(line, row, 'time_us') + " - time_us must be in order, first to last")

                # channel
                if not is_int_in_string(row['channel']):
                    sys.exit(get_error_string(line, row, 'channel') + " - must be 1-4 (wrong type)")

                if int(row['channel']) <= 0 or int(row['channel']) > 4:
                    sys.exit(get_error_string(line, row, 'channel') + " - must be 1-4 (wrong value)")

                # pulse widths
                if not is_int_in_string(row['pos_width_us']):
                    sys.exit(get_error_string(line, row, 'pos_width_us') + " - must integer")

                if int(row['pos_width_us']) < 0 or int(row['pos_width_us']) > 255:
                    sys.exit(get_error_string(line, row, 'pos_width_us') + " - must be 0-255")

                if not is_int_in_string(row['neg_width_us']):
                    sys.exit(get_error_string(line, row, 'neg_width_us') + " - must integer")

                if int(row['neg_width_us']) < 0 or int(row['neg_width_us']) > 255:
                    sys.exit(get_error_string(line, row, 'neg_width_us') + " - must be 0-255")

                # power
                if not is_int_in_string(row['power']):
                    sys.exit(get_error_string(line, row, 'power') + " - must integer")

                if int(row['power']) < 0 or int(row['power']) > 1000:
                    sys.exit(get_error_string(line, row, 'power') + " - must be 0-1000")


                pulses.append(row)
                last_time_us = int(row['time_us'])

                line += 1

        return pulses

    if len(sys.argv) != 2:
        print("Error - must specify pattern file")
        exit()

    csv_filename = sys.argv[1]
    pulse_list = load_pulses_from_csv(csv_filename)

    # Can do "ZcBle(<address>)" instead if the BLE address is known and the connection will be quicker
    ble = ZcBle()

    ble_task = asyncio.create_task(ble.start())

    # wait for connection or connection timeout
    while not ble.connected and not ble_task.done():
        await asyncio.sleep(0.1)

    if not ble.connected:
        return

    # Send a start/clock sync message
    await ble.pulse_send_start()
    start_time_us = ble.time_us()

    # setup
    message_time_offset = 0
    pulse_index = 0
    next_pulse = pulse_list[pulse_index]

    # init debug counters
    debug_packet_count = 0
    debug_message_count = 0
    debug_last_debug_msg_us = ble.time_us()

    print("Start sending pulses")

    while True:
        width_pos_us  = int(pulse_list[pulse_index]['pos_width_us'])
        width_neg_us  = int(pulse_list[pulse_index]['neg_width_us'])
        power         = int(pulse_list[pulse_index]['power'])
        pulse_time_us = message_time_offset + int(pulse_list[pulse_index]['time_us'])
        channel       = int(pulse_list[pulse_index]['channel'])

        await ble.pulse_queue(width_pos_us, width_neg_us, power, pulse_time_us, channel)
        debug_message_count += 1

        pulse_index += 1
        if pulse_index >= len(pulse_list):
            message_time_offset += int(pulse_list[pulse_index-1]['time_us'])
            pulse_index = 0

        # debug output
        if (ble.time_us() - debug_last_debug_msg_us > 1000 * 1000):
            time_s = (ble.time_us() - debug_last_debug_msg_us)/(1000*1000)
            msg_sec = debug_message_count/time_s
            packet_sec = (ble.pq.packets_sent - debug_packet_count) / time_s
            if packet_sec > 0:
                avg_pck_per_msg = "{:.0f}".format(msg_sec / packet_sec)
            else:
                avg_pck_per_msg = "-"

            print("{:.2f}".format(msg_sec) + ' messages/sec, ' + "{:.2f}".format(packet_sec) + ' packets/sec (average ' + avg_pck_per_msg + ' messages/packet)')
            debug_last_debug_msg_us = ble.time_us()
            debug_message_count = 0
            debug_packet_count = ble.pq.packets_sent

        while (int(pulse_list[pulse_index]['time_us'])+message_time_offset > pattern_time_us(start_time_us)):
            await ble.process_queue()
            await asyncio.sleep(0)

asyncio.run(main(address))
