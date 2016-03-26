# Beacon Finder
Little script for a Particle Photon connected to an HM-10 via hardware serial (`Serial1`).

The wiring is quite easy:
- `RX` on Photon goes to `TX` on HM-10
- `TX` on Photon goes to `RX` on HM-10
- `3V3` on Photon goes to `VCC` on HM-10
- `GND` on Photon goes to `GND` on HM-10

I used the "Sounfounder Bluetooth 4.0 HM-10 Master Slave Module".
Every other HM-10 should work also.
