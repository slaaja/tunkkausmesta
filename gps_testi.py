#!/opt/local/bin/python2.7
#!/usr/bin/env python

from gnuradio import gr
from gnuradio import blocks
from gnuradio import digital
from gnuradio import filter
from gnuradio import analog
from gnuradio import audio
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import gps

import osmosdr 
import numpy

import time

import thread

RX_IF = 0
DEFAULT_OSR = 16
DEFAULT_SAMPLE_RATE = DEFAULT_OSR * 1023e3
DEFAULT_RX_FREQ = 1575.42e6 - RX_IF

CODE_SEL = 20
AVG_SEL = 1

class my_topblock(gr.top_block):

    def satellite_search(self,a,b):
        time.sleep(5)
        
        self.despread.start_search(AVG_SEL)
        
        return
        
        print '### Starting search'
        
        delayrange = range(0,1023)
        best_snr = -100
        best_d = 0
        
        
        for d in delayrange:
            self.despread.set_delay(d)
    
            time.sleep(0.1)
            
            snr = self.snrprobe.snr()
            
            print d,': SNR:',snr, 'error=',self.despread.error(),'[best =',best_snr,'at d =',best_d,']'
            
            if snr > best_snr:
                best_snr = snr
                best_d = d
                print '# New best SNR:',best_snr,'with code delay',best_d
                
                
        print '### Search ended. Best SNR =',best_snr,'at d =',best_d
        print '### Setting code delay to d=',best_d
        self.despread.set_delay(best_d)


    def __init__(self, prn_code):
        gr.top_block.__init__(self)
 
        parser = OptionParser(option_class=eng_option)
 
        parser.add_option('-a', '--args', type='string', default='bladerf=0',
                          help='Device args, [default=%default]')
 
        parser.add_option('-s', '--sample-rate', type='eng_float', default=DEFAULT_SAMPLE_RATE,
                          help='Sample rate [default=%default]')
 
        parser.add_option('-f', '--rx-freq', type='eng_float', default=DEFAULT_RX_FREQ,
                          help='RX frequency [default=%default]')
                          
        parser.add_option("", "--ref-scale", type="eng_float", default=1.0,
                         help="Set dBFS=0dB input value, default=[%default]")
 
        parser.add_option("-v", "--verbose", action="store_true", default=False,
                          help="Use verbose console output [default=%default]")
 
        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)
        self.options = options

        self.src = osmosdr.source(options.args)
        
        try:
            self.src.get_sample_rates().start()
        except RuntimeError:
            print "No sample rates. No source connected?"
            sys.exit(1)
            
        
            
        self.src.set_gain(6, 'LNA');
        self.src.set_gain(30, 'VGA1');
        self.src.set_gain(30, 'VGA2');

        for name in self.src.get_gain_names():
            print name, 'gain =', self.src.get_gain(name)

        
        ## signal source
        print "Sample rate:",self.src.set_sample_rate(options.sample_rate)
        self.src.set_bandwidth(10e6)
              
        ## DC compensation
        self.dcb = filter.dc_blocker_cc(512,False)
        self.connect(self.src,self.dcb)
        
        
        #bpf_taps = filter.firdes.complex_band_pass_2(1, 1023e3*16, 0.7e6, 3.3e6, 500e3, 60)
        #self.lpf = filter.fft_filter_ccc(1, bpf_taps)
        #self.connect(self.dcb, self.lpf)
        
        ## AGC
        self.agc = analog.agc_cc()
        self.connect(self.dcb, self.agc)
       
       
        ## DSSS despread
        self.despread=gps.gps_despread(DEFAULT_OSR, 2*3.1415/100)
        self.despread.set_code(prn_code)
        self.connect(self.agc, self.despread)
        print '## PRN code',prn_code,'selected'

        ## BPSK SNR measurement
        self.snrprobe = digital.mpsk_snr_est_cc(digital.SNR_EST_SIMPLE, DEFAULT_OSR*1023*20, 0.1)
        self.connect(self.despread,self.snrprobe)
        
        ## BPSK receiver
        omega = 20
        self.demod = digital.mpsk_receiver_cc(2, 0, 0.5/(1e3*DEFAULT_OSR*2), -10e3, 10e3, 0, 0.05, omega, omega*omega/4, 0.005)
        self.connect(self.snrprobe, self.demod)
        
        #self.c2r = blocks.complex_to_real()
        #self.connect(self.demod, self.c2r)
        
        #self.resampler = filter.fractional_interpolator_ff(0, 44100/50);
        #self.connect(self.c2r, self.resampler)
        
        ## BPSK constellation decoder (f -> b)
        constellation = digital.constellation_bpsk()
        self.constdecoder = digital.constellation_decoder_cb(constellation.base())
        self.connect(self.demod, self.constdecoder)
        
        ## file sink
        #self.outputsink = blocks.file_sink(gr.sizeof_char, './output.raw', True)
        self.outputsink = blocks.file_sink(1, './output.raw', False)
        #self.outputsink = audio.sink(44100)
        self.connect(self.constdecoder, self.outputsink)

        
def main():
    tb = my_topblock(CODE_SEL)
   
    thread.start_new_thread(tb.satellite_search, (0,0))
    tb.run()

   
if __name__ == '__main__':
    main()