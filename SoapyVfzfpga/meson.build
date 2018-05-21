project('SoapyVfzsdr', 'cpp', 'c',
	version : '0.0.1',
	license : 'MIT')

# /usr/local/lib/SoapySDR/modules0.7

soapysdr_dep = dependency('SoapySDR')
alsa_dep = dependency('alsa')

sources = ['SoapyVfzfgpa.cpp', 'alsa.c']
soapy_vfzsdr_lib = shared_library('soapyvfzsdr',
                        sources,
                        dependencies : [soapysdr_dep, alsa_dep],
                        install : true)
