#!/opt/local/bin/python2.7
#!/usr/bin/env python

DEFAULT_RX_FREQ = 1575.42e6
DEFAULT_OSR = 10
DEFAULT_SAMPLE_RATE = 1023e3 * DEFAULT_OSR
SEARCH_DELAY=0.01


DC_COMP_I = 0.2
DC_COMP_Q = 0.42

import osmosdr

from gnuradio import gr, gru, eng_notation, analog, blocks, filter
from gnuradio.gr.pubsub import pubsub
from gnuradio.eng_option import eng_option
from optparse import OptionParser

from gnuradio.wxgui import stdgui2, form, slider, forms, fftsink2, scopesink2,numbersink2
import wx

import sys
import thread
import time

import gps 

import scipy,pylab

KEY_RX_FREQ = "rx_freq"
KEY_DELAY = "delay"
KEY_CODE = "code"
KEY_POWER = "power"
KEY_POWER2 = 'power2'
KEY_BDELAY = "bdelay"
KEY_BFREQ = 'bfreq'
KEY_FREQCORR = 'freqcorr'
GAIN_KEY = lambda x: 'gain: '+x
GAIN_RANGE_KEY = lambda x: 'gain_range: '+x

class top_block(stdgui2.std_top_block, pubsub):
    def __init__(self, frame, panel, vbox, argv):
        stdgui2.std_top_block.__init__(self, frame, panel, vbox, argv)
        pubsub.__init__(self)
 
        self.frame = frame
        self.panel = panel
 
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
        
        self[KEY_RX_FREQ] = options.rx_freq
        self[KEY_CODE] = 1
        self[KEY_DELAY] = 0
        self[KEY_BDELAY] = 0
        self[KEY_POWER] = 0
        self[KEY_POWER2] = 0
        self[KEY_FREQCORR] = 0
        self[KEY_BFREQ] = 0
        
        
        self['dc_offset_real'] = 0
        self['dc_offset_imag'] = 0

        #self.subscribe(KEY_RX_FREQ, self.set_freq)
        self.subscribe('dc_offset_real', self.set_dc_offset)
        self.subscribe('dc_offset_imag', self.set_dc_offset)
     
        #self.subscribe(KEY_DELAY, self.set_delay)
        #self.subscribe(KEY_CODE, self.set_code)
        
        self.src = osmosdr.source(options.args) 
        
        try:
            self.src.get_sample_rates().start()
        except RuntimeError:
            print "Source has no sample rates (wrong device arguments?)."
            sys.exit(1)
        
        ## hommaa gainiarvot ja -nimet src:sta
        for name in self.get_gain_names():
            self.publish(GAIN_KEY(name), (lambda self=self,name=name: self.src.get_gain(name)))
        
        for name in self.get_gain_names():
            self.publish(GAIN_RANGE_KEY(name), (lambda self=self,name=name: self.src.get_gain_range(name)))
        for name in self.get_gain_names():
            self.subscribe(GAIN_KEY(name), (lambda gain,self=self,name=name: self.set_named_gain(gain, name)))
        
        
        
        self.src.set_sample_rate(options.sample_rate)
        self.src.set_bandwidth(4e6)
    
        self.dcb = filter.dc_blocker_cc(512,False)
        self.connect(self.src,self.dcb)
        
        self.despread = gps.gps_despread()
        self.despread.set_osr(DEFAULT_OSR)
        self.connect(self.dcb,self.despread)
            
        
        #self.filtertaps = filter.firdes.low_pass_2(1,options.sample_rate,4000,2000, 50)
        #self.decimator = filter.fft_filter_ccc(186, self.filtertaps)
        #self.connect(self.despread, self.decimator)
        
        
        
        #self.spectrum = fftsink2.fft_sink_c(panel, fft_size=1024, sample_rate=1e3, ref_scale=options.ref_scale,
        #                                    ref_level=0, y_divs=10, average=False, avg_alpha=1e-1,
        #                                  fft_rate=30)
                                #self.spectrum.set_callback(self.wxsink_callback)
        
        #self.scope = scopesink2.scope_sink_c(panel,title='Constellation', sample_rate=1e3, size=(300,300),
        #                                    xy_mode=True)
        #self.connect(self.decimator, self.spectrum)
        #self.connect(self.despread, self.spectrum)
        
        self.powerdet = blocks.rms_cf(alpha=0.002)
        self.connect(self.despread, self.powerdet)
        
        self.powerprobe = blocks.probe_signal_f()
        self.connect(self.powerdet, self.powerprobe)
       
        self.set_freq(options.rx_freq)
        
        self.search_running = False
        
        self.frame.SetMinSize((800, 420))
        self._build_gui(vbox)
        

        
        
        
    def wxsink_callback(self, x, y):
        self.set_freq_from_callback(x)
    
    def get_gain_names(self):
        return self.src.get_gain_names()    
    
    def set_named_gain(self, gain, name):
        if gain is None:
            g = self[GAIN_RANGE_KEY(name)]
            gain = float(g.start()+g.stop())/2

            self[GAIN_KEY(name)] = gain
            return
            
        gain = self.src.set_gain(gain, name)

    
    def set_freq_from_callback(self, freq):
        freq = self.src.set_center_freq(freq)
        self[KEY_RX_FREQ] = freq
        
    def set_freqcorr(self,freqcorr):
        f = self[KEY_RX_FREQ]
        
        self.set_freq(f+freqcorr)
        
    def set_freq(self, freq):
        if freq is None:
            f = self[FREQ_RANGE_KEY]
            freq = float(f.start()+f.stop())/2.0

            return
 
        #self.spectrum.set_center_freq(freq + self[KEY_TX_FREQ_OFF])
        freq = self.src.set_center_freq(freq)
 
        #if hasattr(self.spectrum, 'set_baseband_freq'):
        #    self.spectrum.set_baseband_freq(0)
 

        return freq        
        
    def _build_gui(self, vbox):
        #vbox.Add(self.spectrum.win, 1, wx.EXPAND)
        #vbox.Add(self.scope.win,1,wx.EXPAND)
        vbox.AddSpacer(3)
        #vbox.Add(self.scope.win, 1, wx.EXPAND)
        #vbox.AddSpacer(3)
        
        self.myform = myform = form.form()
        
        box1 = wx.BoxSizer(wx.VERTICAL)
        box1.AddSpacer(3)
        
        self.delay_select_text = forms.text_box(
            parent=self.panel, sizer=box1, label='Code delay', proportion=1, ps=self, key=KEY_DELAY,
            converter=forms.int_converter(),value=0
        )
        
        self.freq_select_text = forms.text_box(
            parent=self.panel, sizer=box1, label='Frequency', proportion=1, ps=self, key=KEY_FREQCORR,
            converter=forms.int_converter(),value=0
        )
        
        self.code_select_text = forms.text_box(
            parent=self.panel, sizer=box1, label='PRN Code [1-31]', proportion=1, ps=self, key=KEY_CODE,
            converter=forms.int_converter(),value=1
        )
        box1.AddSpacer(3)
        self.best_delay_text = forms.text_box(
            parent=self.panel,sizer=box1, label="Best delay", proportion=1, ps=self, key=KEY_BDELAY,
            converter=forms.int_converter(),value=0
        )
        self.highest_power_text= forms.text_box(
            parent=self.panel,sizer=box1, label="Highest power", proportion=1, ps=self, key=KEY_POWER,
            converter=forms.float_converter(),value=0
        )
        
        self.current_power_text=forms.text_box(
            parent=self.panel,sizer=box1, label="Current power", proportion=1, ps=self, key=KEY_POWER2,
            converter=forms.float_converter(),value=0
        )
        
        gc_vbox = forms.static_box_sizer(parent=self.panel,
                                         label="Gain Settings",
                                         orient=wx.VERTICAL,
                                         bold=True)
        gc_vbox.AddSpacer(3)

        # Add gain controls to top window sizer
        vbox.Add(gc_vbox, 0, wx.EXPAND)
        vbox.AddSpacer(5)
        
        for gain_name in self.get_gain_names():
            range = self[GAIN_RANGE_KEY(gain_name)]
            gain = self[GAIN_KEY(gain_name)]

            #print gain_name, gain, range.to_pp_string()
            if range.start() < range.stop():
                gain_hbox = wx.BoxSizer(wx.HORIZONTAL)
                gc_vbox.Add(gain_hbox, 0, wx.EXPAND)
                gc_vbox.AddSpacer(3)

                gain_hbox.AddSpacer(3)
                forms.text_box(
                    parent=self.panel, sizer=gain_hbox,
                    proportion=1,
                    converter=forms.float_converter(),
                    ps=self,
                    key=GAIN_KEY(gain_name),
                    label=gain_name + " Gain (dB)",
                )
                gain_hbox.AddSpacer(5)
                forms.slider(
                    parent=self.panel, sizer=gain_hbox,
                    proportion=3,
                    ps=self,
                    key=GAIN_KEY(gain_name),
                    minimum=range.start(),
                    maximum=range.stop(),
                    step_size=range.step() or (range.stop() - range.start())/10,
                )
                gain_hbox.AddSpacer(3)
        
        

        dc_offset_vbox = forms.static_box_sizer(parent=self.panel,
                                         label="DC Offset Correction",
                                         orient=wx.VERTICAL,
                                         bold=True)
        dc_offset_vbox.AddSpacer(3)
        # First row of sample rate controls
        dc_offset_hbox = wx.BoxSizer(wx.HORIZONTAL)
        dc_offset_vbox.Add(dc_offset_hbox, 0, wx.EXPAND)
        
        vbox.Add(dc_offset_vbox, 0, wx.EXPAND)
        vbox.AddSpacer(3)
        
        dc_offset_hbox.AddSpacer(3)
        self.dc_offset_real_text = forms.text_box(
            parent=self.panel, sizer=dc_offset_hbox,
            label='Real',
            proportion=1,
            converter=forms.float_converter(),
            ps=self,
            key='dc_offset_real',
        )
        dc_offset_hbox.AddSpacer(3)

        self.dc_offset_real_slider = forms.slider(
            parent=self.panel, sizer=dc_offset_hbox,
            proportion=3,
            minimum=-1,
            maximum=+1,
            step_size=0.001,
            ps=self,
            key='dc_offset_real',
        )
        dc_offset_hbox.AddSpacer(3)

        dc_offset_hbox.AddSpacer(3)
        self.dc_offset_imag_text = forms.text_box(
            parent=self.panel, sizer=dc_offset_hbox,
            label='Imag',
            proportion=1,
            converter=forms.float_converter(),
            ps=self,
            key='dc_offset_imag',
        )
        dc_offset_hbox.AddSpacer(3)

        self.dc_offset_imag_slider = forms.slider(
            parent=self.panel, sizer=dc_offset_hbox,
            proportion=3,
            minimum=-1,
            maximum=+1,
            step_size=0.001,
            ps=self,
            key='dc_offset_imag',
        )
        dc_offset_hbox.AddSpacer(3)
        
        
        def search_callback(value):
            if value:
                self.search_running=True
                thread.start_new_thread(self.satellite_search, (SEARCH_DELAY,1))
            else:
                self.search_running=False
        
        self.search_button=forms.toggle_button(
            sizer=box1,parent=self.panel,false_label='Search',true_label='Stop',value=False,callback=search_callback
        )
        
        vbox.Add(box1, 0, wx.EXPAND)

    def set_dc_offset(self, value):
        correction = complex( self['dc_offset_real'], self['dc_offset_imag'] )

        try:
            self.src.set_dc_offset( correction )

        except RuntimeError as ex:
            print ex
    

    def set_code(self,value):
        #print "Setting PRN code to: ", value
        self.despread.set_code(value)
        #self.despread2.set_code(value)
        
    def set_delay(self,value):
        #print "Setting PRN code delay to ", value
        self.despread.set_delay(value)
        
    def satellite_search(self, searchdelay,a):
        #self.delay_select_text.Disable()
        #self.code_select_text.Disable()
        
        searchcode = self[KEY_CODE]
        
        print "#### Searching for GPS satellite with PRN code", searchcode
        
        self.set_code(searchcode)
        
        d = 0

        f_step = 1000
        
        codes = range(0, 1023-1)
        freqs = range(-4000,4000,f_step)
        powers = [[0 for item in range(0,len(codes))] for item in range(0,len(freqs))]
        

        for d in range(0,len(codes)):
            for i in range(0,len(freqs)):
                
                if self.search_running == False:
                    print "#### Search ended"
                    thread.exit()
                
                self.set_freq(DEFAULT_RX_FREQ + freqs[i])
                self[KEY_FREQCORR] = freqs[i]
          
                self.set_delay(d*DEFAULT_OSR)
                self[KEY_DELAY] = d
                
 
                
                time.sleep(searchdelay)

                p = max(max(powers))
                powers[i][d] = self.powerprobe.level()
                self[KEY_POWER2] =(20*scipy.log10(self.powerprobe.level())) 
            
                if max(max(powers)) > p :
                        
                    self[KEY_BDELAY] = (d)
                    self[KEY_BFREQ] = freqs[i]
                    self[KEY_POWER] = (20*scipy.log10(max(max(powers))))
                        #print 'new best freq offset',freqs[i], 'Hz'
                        
        print '##### Search complete'
        self.set_freq(DEFAULT_RX_FREQ + self[KEY_BFREQ])
        self.set_delay(self[KEY_BDELAY]*DEFAULT_OSR)

        thread.exit()

            
def main():
    app = stdgui2.stdapp(top_block, 'GPS receiver', nstatus=1)
    app.MainLoop()
 
if __name__ == '__main__':
    main ()
