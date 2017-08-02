## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
    module = bld.create_ns3_module('epidemic', ['internet'])
    module.includes = '.'
    module.source = [
        'model/epidemic-packet-queue.cc',
        'model/epidemic-packet.cc',
        'model/epidemic-tag.cc',
        'model/epidemic-routing-protocol.cc',
        'helper/epidemic-helper.cc',
        ]
        
    module_test = bld.create_ns3_module_test_library('epidemic')
    module_test.source = [
        'test/epidemic-test-suite.cc',
        ]
        
    headers = bld(features='ns3header')
    headers.module = 'epidemic'
    headers.source = [
        'model/epidemic-packet-queue.h',
        'model/epidemic-packet.h',
        'model/epidemic-tag.h',
        'model/epidemic-routing-protocol.h',
        'helper/epidemic-helper.h',
        ]


    if bld.env['ENABLE_EXAMPLES']:
        bld.recurse('examples')

    bld.ns3_python_bindings()
