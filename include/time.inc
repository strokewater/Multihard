%ifndef TIME_INC
%define TIME_INC


%define	SCHED_FREQ		100
%define PIT_PERIOD_US		(1 * 1000 * 1000  / SCHED_FREQ )
; Timer 0 Frequency: 100 HZ, 0.01s=10ms a timer.

%endif