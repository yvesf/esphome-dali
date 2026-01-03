# esphome-dali

A simple esphome integration for light brightness/on/off control of DALI lights using the LW14 adapter.

## Example

```yaml
dali:
    id: dali_bus

light:
  - name: Küche
    id: light0
    platform: dali
    bus: dali_bus
    short_address: 0
  - name: Arbeitszimmer
    id: light1
    platform: dali
    bus: dali_bus
    short_address: 1
```

## Similar code
- https://github.com/jorticus/esphome-dali
  - Much more complete but also more complicated to use.
  - Should be possible to adapt to use with LW14, but I couldn't get it to work.

## Documents
### Dali
- https://sitelec.org/download.php?filename=themes/showroom/resume_commandes_dali.pdf
- https://jared.geek.nz/2025/06/dali-lighting-protocol/
- https://github.com/qqqlab/DALI-Lighting-Interface/blob/main/qqqDALI.cpp
- https://github.com/sde1000/python-dali
- https://www.phocos.com/wp-content/uploads/2025/10/CIS-N-MPPT-DALI_Memory_Bank_Map_v1.3_2025-10-03.pdf
- https://cdn.standards.iteh.ai/samples/23720/e5f17dc3209f40dabe2569f0b37d46a2/IEC-62386-103-2014-AMD1-2018.pdf
- DIN EN 62386-102 / 207.

### LW14
- https://github.com/codemercs-com/lw14
- https://www.codemercs.com/downloads/ledwarrior/LW14_Datasheet.pdf
- https://codemercs.com/downloads/ledwarrior/LW11_Datasheet.pdf

### LED Drivers
- https://bokedriver.com/wp-content/uploads/2024/01/Datasheet-CC-dimmable-driver-BK-DEN-Series-suffix-d-V1.2-EN.pdf

### esphome
- https://github.com/esphome/esphome/blob/dev/esphome/components/light/light_state.cpp

