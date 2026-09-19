# LineEndApproachTracer

Connects ET Rally to ET Sumo by approaching the vertical marker line, turning
toward its upper end, following it, and stopping at its endpoint. On the R course,
start near the upper-left QR, facing left. The initial turn is left by 90 degrees.
L-course selection mirrors the turn and tracking edge automatically.

## Configuration

```text
LineEndApproachTracer targetBrightness pwm maxPwm turnPwm edge kp ki kd [key=value ...]
LineEndApproachTracer 55 50 80 50 RIGHT_EDGE 0.6 0.01 0.017 qrClearMm=40 approachMaxMm=500 sensorOffsetMm=45 traceMaxMm=700 stableMm=100 gapMm=20 markerMaxMm=80 scanDeg=20 endOffsetMm=0 timeoutTicks=10000
```

All distances are millimeters and are provisional until measured on the robot.
The brightness target is normalized to the calibrated black/white range.
PID uses raw reflection errors against that calibrated target, like LineTracer.
Straight motion, marker crossing and final positioning use `pwm` (50 in the
example), with the existing encoder correction. There is no
automatic low-speed approach. `maxPwm` caps PID wheel outputs; encoder correction
can adjust straight outputs by the Walker's existing 20 percent margin.
The initial turn and alignment sweeps use `turnPwm` (50 in the example).

| Option | Default | Meaning |
| --- | ---: | --- |
| qrClearMm | 40 | Ignore initial QR/marker detections until this approach distance. |
| approachMaxMm | 500 | Maximum straight approach distance before failure. |
| sensorOffsetMm | 45 | Forward distance from wheel axle to floor sensor; advance this far before turning. |
| traceMaxMm | 700 | Maximum net forward encoder distance after alignment. |
| stableMm | 30 | Required uninterrupted non-white, non-marker tracking before accepting an endpoint. The current scenario uses 100. |
| gapMm | 20 | Straight travel after first white detection, before accepting the endpoint. |
| markerMaxMm | 80 | Maximum marker bypass distance, including black-line reacquisition. |
| scanDeg | 20 | Sweep half-angle around the current heading (0 < value <= 45). |
| endOffsetMm | 0 | Signed final forward offset from the axle position at first white detection. |
| timeoutTicks | 10000 | Maximum run calls after starting; includes all motion phases. |

With `endOffsetMm=0`, the sensor is approximately at the endpoint and the axle
remains behind it. To put the axle approximately at the endpoint, set
`endOffsetMm` to the measured sensor offset, then tune for detection and braking
overshoot. The final heading is restored to the last tracking heading, not an
absolute compass heading.

Unknown keys, missing arguments, non-numeric values and out-of-range parameters
produce a brake-and-hold step, rather than silently skipping into ET Sumo.

## Motion And Failure Handling

1. Ignore the starting QR for `qrClearMm`. Require a white observation beyond
   that distance before accepting two consecutive black/colored detections.
2. Advance by `sensorOffsetMm`, turn using the existing RotateTracer calibration,
   then drive straight 50 mm to leave the colored marker before searching within
   `scanDeg` for the black line.
3. Follow the edge using PID. On blue/red/yellow/green, retain the current heading
   with encoder-corrected straight motion until black is reacquired twice.
   Blue is optional: joining above blue does not require reversing to find it.
4. At normalized reflection >= 85 without a marker, do not accept an endpoint
   until the line has been tracked continuously for `stableMm`. Before that, keep
   driving straight so a short crossing line cannot become the endpoint.
5. After stable tracking, travel `gapMm` straight; if the line returns, resume
   tracking. Otherwise, move directly back to the first-white position plus
   `endOffsetMm`, brake, and allow the next tracer to start.

Failure latches with `[LINE_END] failed: ...`, brakes on every subsequent run,
and does **not** report normal termination. The following ET Sumo step cannot
start. Use the existing left-button stop to exit. Logs also record phase changes;
numeric phases follow the Phase enum in the header.

The approach assumes the starting height intersects the vertical line and that
`qrClearMm` ends after the QR but before the line. QR and line cannot be identified
from a single black sample. A start above the physical line endpoint currently
fails at the approach limit; automatic lower-row retries are not implemented.
The endpoint test is bounded evidence, not proof: a gap longer than `gapMm` can
resemble an endpoint. Choose bounds from actual geometry. Colored markers must
have enough black line after them for stable tracking. Sensor lateral offset,
slip, braking distance and marker width need hardware verification.

## Scenario Placement

Place this step after a Rally route ending at the upper-left QR facing left,
before ET Sumo's heading adjustment. The existing early `#end` in tracer.ini is
intentionally preserved, so the added step is not active in the current scenario.
For a standalone test, temporarily make this the first active step followed by
`#end`, and physically place the robot at the assumed start pose. For a full run,
configure/enable the Rally route and remove the early stop deliberately.

## Validation

Run from `~/etrobo`:

```sh
g++ -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
    -Iworkspace/EtRobocon2026/app -Iworkspace/EtRobocon2026/unit \
    -Ispike-rt/drivers/include/libcpp/spike \
    workspace/EtRobocon2026/tests/line_end_approach_test.cpp \
    -o /tmp/line_end_approach_test
/tmp/line_end_approach_test
make app=EtRobocon2026
```

Tests substitute sensor and encoder readings while executing the actual tracer,
PID, color classifier, and RotateTracer implementation. They cover joining on
black/blue, endpoint confirmation, temporary loss/reacquisition, failure latching,
timeouts, mirrored setup, scan overshoot and configuration validation. They do
not simulate physical steering, braking or color-sensor accuracy.