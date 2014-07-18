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

#include <gps/gps_despread.h>

namespace gr {
  namespace gps {

    class gps_despread_impl : public gps_despread
    {
    private:

		// code generator
		gr_complex code_LUT[1023];
		int code_selection;
		int code_counter;
		 
		int osr_int;
		int osr_counter;
		 
		int delay_selection;
		 
		int delay_next; 
		 
		 // search
		int search_enabled;
		int search_avg_counter;
		int search_avg_selection;
		gr_complex search_avg_accumulator;
		int search_counter;
		float avg_accu;
		 
		int best_delay;
		float best_power;
		
		int tracking_enabled;
		 
		// sample accumulators
		gr_complex output_sample;
		gr_complex output_sample_early;
		gr_complex output_sample_late;

	public:
		gps_despread_impl(int);
		~gps_despread_impl();

		void set_code(int);
		int code(void) const;
	  
		void set_osr(int);
		int osr(void) const;
	  
		void set_delay(int);
		int delay(void) const;
	  
	  	float peak(void) const;
		
		int search_running(void) const;
	  
		void start_search(int);
	  
		void forecast(int, gr_vector_int &);
	  
      // Where all the action really happens
		int general_work(int, gr_vector_int &, gr_vector_const_void_star &,
                       gr_vector_void_star &);
    };

  } // namespace gps
} // namespace gr

#endif /* INCLUDED_GPS_GPS_DESPREAD_IMPL_H */

