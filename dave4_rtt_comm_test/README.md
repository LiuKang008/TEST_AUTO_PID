# DAVE4 / XMC4200 RTT communication test

This folder contains a minimal SEGGER RTT text communication test for a DAVE4 project.

## What it verifies

1. XMC4200 prints CSV data through SEGGER RTT channel 0.
2. The PC-side `llm-pid-tuner/tools/rtt_comm_test.py` can read those lines.
3. The PC-side script can send text commands through RTT down channel 0.
4. XMC4200 receives those commands with `SEGGER_RTT_Read()` and prints ACK lines.

## Files

- `rtt_comm_test.h`
- `rtt_comm_test.c`

## DAVE4 integration

1. Make sure your project already contains SEGGER RTT files:
   - `SEGGER_RTT.c`
   - `SEGGER_RTT.h`
   - `SEGGER_RTT_printf.c`, if your project uses `SEGGER_RTT_printf()`
   - `SEGGER_RTT_Conf.h`

2. Copy `rtt_comm_test.c` and `rtt_comm_test.h` into your DAVE4 project.

3. Include the header in your main file:

```c
#include "rtt_comm_test.h"
```

4. Call once after DAVE APP initialization:

```c
RTT_CommTest_Init();
```

5. Call periodically from your main loop or a timer task:

```c
while (1)
{
    RTT_CommTest_Task_Periodic(1u);  /* if this loop/tick is approximately 1 ms */
}
```

If you already have a 1 ms task, call:

```c
RTT_CommTest_Task_1ms();
```

## Expected output

The target prints lines similar to:

```text
# DAVE4 XMC RTT communication test ready
# CSV: timestamp_ms,setpoint,input,pwm,error,p,i,d
500,100.000,37.700,63.653,62.300,1.00000,0.10000,0.01000
1000,100.000,51.400,49.923,48.600,1.00000,0.10000,0.01000
```

## PC-side test

From the `llm-pid-tuner` repository:

```bash
python tools/rtt_comm_test.py --host 127.0.0.1 --port 19021 --parse-csv
```

The default commands sent by the script are:

```text
PING
STATUS
SET P:1.2000 I:0.0300 D:0.0000
STATUS
```

The target should respond with lines similar to:

```text
# ACK PONG ms=1234 rx=1
# STATUS ms=2345 rx=2 err=0 set=100.000 input=55.000 pwm=45.000 P=1.00000 I=0.10000 D=0.01000
# ACK SET P=1.20000 I=0.03000 D=0.00000
```

## Important notes

- Close J-Link RTT Client / RTT Viewer if the Python script cannot connect to `127.0.0.1:19021`.
- This is only a communication smoke test. It does not replace your real PID loop yet.
- After the RTT read/write path is verified, the next step is to replace `SerialBridge` with `RttTelnetBridge` in the real hardware tuning path.
