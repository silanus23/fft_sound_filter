# Drivers/ — vendored third-party code

Nothing in this directory was written by me. It is vendored so that the repository
builds with `make all` from a clean clone, with no setup step.

All of it comes from **[STM32CubeF4 v1.28.3](https://github.com/STMicroelectronics/STM32CubeF4/releases/tag/v1.28.3)**
and its pinned submodules:

| Path | Upstream | Version / commit |
|---|---|---|
| `STM32F4xx_HAL_Driver/` | [STMicroelectronics/stm32f4xx_hal_driver](https://github.com/STMicroelectronics/stm32f4xx_hal_driver) | `b6f0ed3829f3829eb358a2e7417d80bba1a42db7` |
| `CMSIS/Device/ST/STM32F4xx/` | [STMicroelectronics/cmsis_device_f4](https://github.com/STMicroelectronics/cmsis_device_f4) | `3c77349ce04c8af401454cc51f85ea9a50e34fc1` |
| `CMSIS/Include/` | ARM CMSIS Core (via STM32CubeF4) | CMSIS 5 |
| `CMSIS/DSP/` | ARM CMSIS-DSP (via STM32CubeF4) | V1.10.0 |

## What was trimmed, and why

The full `Drivers/` tree from STM32CubeF4 is ~86 MB. Two parts were reduced to keep
the repository a reasonable size; nothing that is compiled or included was removed.

- **`CMSIS/Device/ST/STM32F4xx/Include/`** — upstream ships a ~1 MB register header for
  every STM32F4 variant (~24 MB total). Only the three needed to build for this part are
  kept: `stm32f4xx.h`, `stm32f407xx.h`, `system_stm32f4xx.h`. The build defines
  `STM32F407xx`, so `stm32f4xx.h` only ever includes `stm32f407xx.h`.
- **`CMSIS/DSP/Source/`** — upstream ships 15 function groups (~12 MB). Only the five the
  Makefile compiles from are kept: `TransformFunctions`, `CommonTables`,
  `BasicMathFunctions`, `SupportFunctions`, `FastMathFunctions`.

The HAL driver (`Inc/` and `Src/`) and the CMSIS core headers are complete and unmodified.

## Licensing

This code is licensed by STMicroelectronics and ARM, not by me. See the licence files
kept alongside each component:

- `STM32F4xx_HAL_Driver/LICENSE.md`
- `CMSIS/Device/ST/STM32F4xx/LICENSE.md`
- `CMSIS/LICENSE.txt`

## Reproducing this directory

```sh
git clone --depth 1 --filter=blob:none --sparse \
    --branch v1.28.3 https://github.com/STMicroelectronics/STM32CubeF4 cubef4
cd cubef4
git sparse-checkout set Drivers
git submodule update --init --depth 1 \
    Drivers/STM32F4xx_HAL_Driver Drivers/CMSIS/Device/ST/STM32F4xx
```

Then copy the subtrees listed above.
