/* -*- c++ -*- */
/* 
 * Copyright 2014 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_GPS_GPS_DESPREAD_IMPL_H
#define INCLUDED_GPS_GPS_DESPREAD_IMPL_H

#define DEBUG_OUT
#define MAX_SEARCH_THREADS 4

#include <gps/gps_despread.h>
#include <gnuradio/fft/fft.h>

void max_abs(gr_complex *b, int l, float &a, int &ix, float &avg)
{
	unsigned long i = 0;

	avg = 0;
	a = 0;
	ix = 0;
	
	for(i = 0;i < l; ++i)
	{
		avg += abs(b[i]);
		if(abs(b[i]) > a)
		{
			a = abs(b[i]);
			ix = i;
		}
	}
	
	avg = avg / l;
	
}

void calculate_product(gr_complex *in, gr_complex *c, gr_complex *out, int l, int shift )
{
	int i = 0;
	
	for(i = 0; i < l; ++i)
	{
		int code_ix = i - shift;
		
		while(code_ix < 0)
		{
			code_ix = code_ix + l;
		}
		
		while(code_ix >= l)
		{
			code_ix = code_ix - l;
		}
		
		out[i] = in[i] * c[code_ix];		
	}
	
}


struct input_data 
{
	int started;
	int running;
	
	int osr;
	
	int freq_search_Nsteps;
	
	int len;
	gr_complex *buffer;
	gr_complex spreading_code[1023];
	
	
	
	float best_freq;
	float best_power;
	int best_delay;
};



namespace gr {
  namespace gps {

    class gps_despread_impl : public gps_despread
    {
	private:

		static void *search(void *data_in)
		{
			int code_counter_int = 0;
			int d = 0;
			int f = 0;
			int i;
	
			struct input_data *data = (struct input_data *)data_in;
		
			data->started = 1;
			data->running = 1;
		
			data->best_delay = 0;
			data->best_power = -1;
			data->best_freq = 0;
			
			

		#ifdef DEBUG_OUT
			printf("### Search thread started\n");
		#endif
			
			FILE *fid = fopen("debug_out.txt", "w");
			
			for(i = 0;i < data->len;++i)
				fprintf(fid,"%f,%f,\n", real(data->buffer[i]), imag(data->buffer[i]));
			fclose(fid);
			
			gr::fft::fft_complex *fft_c = new gr::fft::fft_complex(data->len, 1, 4);
			
			
			printf("# First plan done...\n");
			
			gr_complex *input_buffer = fft_c->get_inbuf();
			gr_complex *output_buffer = fft_c->get_outbuf();
			
			
			//
			// calculate fft of input data and spreading code
			//
			gr_complex *code_fft_conj = new gr_complex[data->len];
			gr_complex *input_data_fft = new gr_complex[data->len];
			
			for(i = 0; i < data->len; ++i)
			{
				input_buffer[i] = data->spreading_code[((int)(i/data->osr)) % 1023];
			}

			fft_c->execute();
			
			for(i = 0; i < data->len; ++i)
			{
				code_fft_conj[i] = conj(output_buffer[i]);
				input_buffer[i] = data->buffer[i];
			}
			
			fft_c->execute();
			
			for(i = 0; i < data->len ; ++i)
			{
				input_data_fft[i] = output_buffer[i];
			}
			
			printf("# Static transforms calculated.\n");
			
			float freq_step = 1023e3 * data->osr / data->len;
	
			delete fft_c;
			gr::fft::fft_complex *ifft_c = new gr::fft::fft_complex(data->len, 0, 4);
	
			input_buffer = ifft_c->get_inbuf();
			output_buffer = ifft_c->get_outbuf();
				
				
			printf("# Calculating IFFTs.\n");
			printf("freq_step = %f\n", freq_step);
			printf("freq_search_Nsteps = %d\n", data->freq_search_Nsteps);
			
			for(i = -(data->freq_search_Nsteps >> 1) ; i < (data->freq_search_Nsteps >> 1) ; ++i)
			{
				calculate_product(input_data_fft, code_fft_conj, input_buffer, data->len, i);
					
				ifft_c->execute();
					
				float peak;
				int peak_offset;
				float avg;
					
				max_abs(output_buffer, data->len, peak, peak_offset, avg);
					
				if(peak > data->best_power)
				{
						
					data->best_power = peak;
					data->best_freq = i * freq_step;
					data->best_delay = peak_offset;
							
							
					printf("i: %d, avg=%f, peak=%f, best_power=%f, best_freq=%f, best_delay=%d\n",i, avg, peak, data->best_power, data->best_freq, data->best_delay);
					
				
				}
				
				
					
			}

			delete ifft_c;
		#ifdef DEBUG_OUT
			printf("### Search thread ended\n");
		#endif
		
			data->running = 0;
		
			delete code_fft_conj;
			delete input_data_fft;
			
			delete data->buffer;

			pthread_exit(NULL);
		}
		
		
		
		// code generator
		gr_complex code_LUT[1023];
		int code_selection;
		int code_counter;
		int increment_counter;
		 
		int osr_int;
		int osr_counter;
		 
		int delay_selection;

		 
		 // search
		int search_enabled;
		int search_avg_selection;

		int code_changed;
		
		int freq_search_Nsteps;
		float freq_search_start;
		float freq_search_step;
		float nco_freq_search;
		float best_freq;
		
		pthread_t search_thread;
		
		struct input_data data;
		 
		unsigned long search_buffer_fill_counter;
		gr_complex *buffer; 
		 
		// doppler correction
		float nco_freq;
		float nco_phase;
		
		// frequency tracking
		float freq_track_step;
		float nco_freq_track[4];
		float nco_phase_track[4];
		gr_complex freq_track_integrator[5];
		// code tracking
		gr_complex code_track_integrator[5];

		int find_max_code() const;
		int find_max_freq() const;

	public:
		gps_despread_impl(int, float);
		~gps_despread_impl();

		void set_code(int);
		int code(void) const;
	  
		void set_osr(int);
		int osr(void) const;
	  
		void set_delay(int);
		int delay(void) const;
	  
	  	float peak(void) const;
		float error(void) const;
		
		int search_running(void) const;
	  
		void start_search(int);
		
		float (gps_despread_impl::*d_phase_detector)(gr_complex sample) const;
	
      // Where all the action really happens
		int work(int, gr_vector_const_void_star &, gr_vector_void_star &);
    };

  } // namespace gps
} // namespace gr

#endif /* INCLUDED_GPS_GPS_DESPREAD_IMPL_H */

