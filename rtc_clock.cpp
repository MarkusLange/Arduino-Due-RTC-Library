#include "rtc_clock.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Arduino.h"

RTC_clock::RTC_clock(int source)
{
	_source = source;
	
	if (_source) {
		pmc_switch_sclk_to_32kxtal(0);
	
		while (!pmc_osc_is_ready_32kxtal());
	}
}

void RTC_clock::init()
{
	RTC_SetHourMode(RTC, 0);
	
	NVIC_DisableIRQ(RTC_IRQn);
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_SetPriority(RTC_IRQn, 0);
//	NVIC_EnableIRQ(RTC_IRQn);
//	RTC_EnableIt(RTC, RTC_IER_SECEN | RTC_IER_ALREN);
//	RTC_EnableIt(RTC, RTC_IER_SECEN);
}

void RTC_clock::set_time(int hour, int minute, int second)
{
	_hour = hour;
	_minute = minute;
	_second = second;
	RTC_SetTime (RTC, _hour, _minute, _second);
}

uint32_t RTC_clock::current_time()
{
uint32_t dwTime;

/* Get current RTC time */
dwTime = RTC->RTC_TIMR ;
while ( dwTime != RTC->RTC_TIMR )
	{
	dwTime = RTC->RTC_TIMR ;
	}
	return (dwTime);
}

int RTC_clock::get_hours()
{
	_current_time = current_time();
	
	return (((_current_time & 0x00300000) >> 20) * 10 + ((_current_time & 0x000F0000) >> 16));
}

int RTC_clock::get_minutes()
{
	_current_time = current_time();
	
	return (((_current_time & 0x00007000) >> 12) * 10 + ((_current_time & 0x00000F00) >> 8));
}

int RTC_clock::get_seconds()
{
	_current_time = current_time();
	
	return (((_current_time & 0x00000070) >> 4) * 10 + ((_current_time & 0x0000000F)));
}

/**
 * \brief Calculate day_of_week from year, month, day.
 */
int RTC_clock::calculate_day_of_week(uint16_t _year, int _month, int _day)
{
	int _week;

	if (_month == 1 || _month == 2) {
		_month += 12;
		--_year;
	}

	_week = (_day + 2 * _month + 3 * (_month + 1) / 5 + _year + _year / 4 - _year / 100 + _year / 400) % 7;

	++_week;

	return _week;
}

void RTC_clock::set_date (int day, int month, uint16_t year)
{
	_day = day;
	_month = month;
	_year = year;
	_day_of_week = calculate_day_of_week(_year, _month, _day);
	RTC_SetDate (RTC, (uint16_t)_year, (uint8_t)_month, (uint8_t)_day, (uint8_t)_day_of_week);
}

uint32_t RTC_clock::current_date()
{
uint32_t dwTime;

/* Get current RTC time */
dwTime = RTC->RTC_CALR ;
while ( dwTime != RTC->RTC_CALR )
	{
	dwTime = RTC->RTC_CALR ;
	}
	return (dwTime);
}

uint16_t RTC_clock::get_years()
{
	_current_date = current_date();
	
	return ((((_current_date >> 4) & 0x7) * 1000) + ((_current_date & 0xF) * 100)
  						+ (((_current_date >> 12) & 0xF) * 10) + ((_current_date >> 8) & 0xF));
}

int RTC_clock::get_months()
{
	_current_date = current_date();
	
	return ((((_current_date >> 20) & 1) * 10) + ((_current_date >> 16) & 0xF));
}

int RTC_clock::get_days()
{
	_current_date = current_date();
	
	return ((((_current_date >> 28) & 0x3) * 10) + ((_current_date >> 24) & 0xF));
}

int RTC_clock::get_day_of_week()
{
	_current_date = current_date();
	
	return (((_current_date >> 21) & 0x7));
}

int RTC_clock::set_hours (int hour)
{
	_hour = hour;
	uint32_t _current_time = current_time();
	uint32_t _changed;
	
	_changed = ((_hour%10) | ((_hour/10)<<4))<<16 ;
	
	_current_time = (_current_time & 0xFFC0FFFF) ^ _changed ;
	
	change_time(_current_time);
}

int RTC_clock::set_minutes (int minute)
{
	_minute = minute;
	uint32_t _current_time = current_time();
	uint32_t _changed;
	
	_changed = ((_minute%10) | ((_minute/10)<<4))<<8 ;
	
	_current_time = (_current_time & 0xFFFF80FF) ^ _changed ;
	
	change_time(_current_time);
}

int RTC_clock::set_seconds (int second)
{
	_second = second;
	uint32_t _current_time = current_time();
	uint32_t _changed;
	
	_changed = ((_second%10) | ((_second/10)<<4)) ;
	
	_current_time = (_current_time & 0xFFFFFF80) ^ _changed ;
	
	change_time(_current_time);
}

uint32_t RTC_clock::change_time (uint32_t now)
{
	_now = now;
	
	RTC->RTC_CR |= RTC_CR_UPDTIM ;
	while ((RTC->RTC_SR & RTC_SR_ACKUPD) != RTC_SR_ACKUPD) ;
	RTC->RTC_SCCR = RTC_SCCR_ACKCLR ;
	RTC->RTC_TIMR = _now ;
	RTC->RTC_CR &= (uint32_t)(~RTC_CR_UPDTIM) ;
	RTC->RTC_SCCR |= RTC_SCCR_SECCLR ;
		
	return (int)(RTC->RTC_VER & RTC_VER_NVTIM) ;
}

int RTC_clock::set_days (int day)
{
	_day = day;
	uint32_t _current_date = current_date();
	uint32_t _changed;
	
	_day_of_week = calculate_day_of_week(get_years(), get_months(), _day) ;
	_day_of_week = ((_day_of_week%10) | (_day_of_week/10)<<4)<<21 ;
	
	_changed = ((_day%10) | (_day/10)<<4)<<24 ;
	
	_current_date = (_current_date & (0xC0FFFFFF & 0xFF1FFFFF) ) ^ ( _changed | _day_of_week ) ;
	
	change_date(_current_date);
}

int RTC_clock::set_months (int month)
{
	_month = month;
	uint32_t _current_date = current_date();
	uint32_t _changed;
	
	_day_of_week = calculate_day_of_week(get_years(), _month, get_days()) ;
	_day_of_week = ((_day_of_week%10) | (_day_of_week/10)<<4)<<21 ;
	
	_changed = ((_month%10) | (_month/10)<<4)<<16 ;
	
	_current_date = (_current_date & (0xFFE0FFFF & 0xFF1FFFFF) ) ^ ( _changed | _day_of_week ) ;
	
	change_date(_current_date);
}

int RTC_clock::set_years (uint16_t year)
{
	_year = year;
	uint32_t _current_date = current_date();
	uint32_t _changed;
	
	_day_of_week = calculate_day_of_week(_year, get_months(), get_days()) ;
	_day_of_week = ((_day_of_week%10) | (_day_of_week/10)<<4)<<21 ;
		
	_changed = (((_year/100)%10) | ((_year/1000)<<4)) | ((_year%10) | (((_year/10)%10))<<4)<<8 ;
	
	_current_date = (_current_date & (0xFFFF0080 & 0xFF1FFFFF) ) ^ ( _changed | _day_of_week ) ;
	
	change_date(_current_date);
}

uint32_t RTC_clock::change_date (uint32_t now)
{
	_now = now;
	
	RTC->RTC_CR |= RTC_CR_UPDCAL ;
	while ((RTC->RTC_SR & RTC_SR_ACKUPD) != RTC_SR_ACKUPD) ;
	RTC->RTC_SCCR = RTC_SCCR_ACKCLR ;
	RTC->RTC_CALR = _now ;
	RTC->RTC_CR &= (uint32_t)(~RTC_CR_UPDCAL) ;
	RTC->RTC_SCCR |= RTC_SCCR_SECCLR ;
		
	return (int)(RTC->RTC_VER & RTC_VER_NVCAL) ;
}

void (*useralarmFunc)(void);

void RTC_clock::attachalarm(void (*userFunction)(void))
{
	useralarmFunc = userFunction;
}

void RTC_Handler(void)
{
	uint32_t status = RTC->RTC_SR;
	
	/* Time or date alarm */
	if ((status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		/* Disable RTC interrupt */
		RTC_DisableIt(RTC, RTC_IDR_ALRDIS);
		
		useralarmFunc();

		/* Clear notification */
		RTC_ClearSCCR(RTC, RTC_SCCR_ALRCLR);
		RTC_EnableIt(RTC, RTC_IER_ALREN);
	}
}

void RTC_clock::set_alarmtime(int hour, int minute, int second)
{
	uint8_t _hour = hour;
	uint8_t _minute = minute;
	uint8_t _second = second;
	
	RTC_EnableIt(RTC, RTC_IER_ALREN);
	RTC_SetTimeAlarm(RTC, &_hour, &_minute, &_second);
	NVIC_EnableIRQ(RTC_IRQn);
}

void RTC_clock::set_alarmdate(int day, int month)
{
	uint8_t _month = month;
	uint8_t _day = day;
	
	RTC_EnableIt(RTC, RTC_IER_ALREN);
	RTC_SetDateAlarm(RTC, &_month, &_day);
	NVIC_EnableIRQ(RTC_IRQn);
}

void RTC_clock::disable_alarm()
{
	RTC_DisableIt(RTC, RTC_IDR_ALRDIS);
}