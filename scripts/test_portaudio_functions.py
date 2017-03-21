#!/usr/bin/env python
# encoding: utf-8
from pyo import *

print("------------- PA Version ---------------------")
print('pa_get_version:', pa_get_version())
print('pa_get_version_text:', pa_get_version_text())

print("------------- Host API ---------------------")
print('pa_count_host_apis:', pa_count_host_apis())
print('pa_list_host_apis:')
pa_list_host_apis()
print('pa_get_default_host_api:', pa_get_default_host_api())

print("--------------- Devices -------------------")
print('pa_count_devices:', pa_count_devices())
print('pa_list_devices:')
pa_list_devices()
print('pa_get_input_devices:')
print(pa_get_input_devices())
print('pa_get_default_input:', pa_get_default_input())
print('pa_get_output_devices:')
print(pa_get_output_devices())
print('pa_get_default_output:', pa_get_default_output())

print("--------------- Device Infos -------------------")
print('pa_get_devices_infos:')
pa_get_devices_infos()

print("--------------- Channels -------------------")
dev_list, dev_index =  pa_get_output_devices()
for dev in dev_index:
    print('Device index:', dev, 'Max outputs:', pa_get_output_max_channels(dev))

dev_list, dev_index =  pa_get_input_devices()
for dev in dev_index:
    print('Device index:', dev, 'Max inputs:', pa_get_input_max_channels(dev))
