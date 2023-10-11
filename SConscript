from building import *
import os

cwd = GetCurrentDir()

src = Split('''
src/ralarm.c
adapter/rtthread/ral_task_adapter.c
adapter/rtthread/ral_mutex_adapter.c
adapter/rtthread/ral_event_adapter.c
''')

CPPPATH = [cwd + '/include']

if GetDepend(['PKG_USING_RALARM_EXAMPLE']):
    src += ['example/ralarm_example.cpp']

group = DefineGroup('ralarm', src, depend = [''], CPPPATH = CPPPATH)
Return('group')
