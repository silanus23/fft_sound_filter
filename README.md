# STM32F407 Real-Time Audio DSP System

This repository is a learning experience. Finding chip register addresses — particularly the undocumented CS43L22 initialization sequence — and implementing real-time DSP via CMSIS-DSP were new territory. The codec requires a specific sequence of magic values that aren't fully explained in the datasheet, just referenced in application notes. CMSIS-DSP turned out to be straightforward once the FFT instance setup and buffer alignment were sorted.

The analog side behaved as expected — shared power supply noise, jumper wires picking up EMI, the usual compromises of a breadboard prototype. A clean external supply and shielded cable would fix it, but that's a hardware problem not a software one.

## Hardware

Audio input is a MAX4466 microphone amplifier connected to PA1. Getting clean signal out of it is harder than it looks — it's sensitive to power supply noise and picks up EMI through unshielded jumper wires. Use short connections and power it from a clean supply if possible, not directly from the Discovery board's 3.3V.

Audio output is the onboard CS43L22 codec — headphones into the 3.5mm jack on the Discovery board, nothing extra needed.

| Pin | Function |
|-----|----------|
| PA1 | MAX4466 OUT (analog input) |
| PD4 | CS43L22 reset |
| PB6/PB9 | I2C (codec control) |
| PA4/PC7/PC10/PC12 | I2S (audio data) |

## How It Works

Processing audio in chunks creates two problems. First, cutting a signal into finite blocks causes frequency energy to spill into neighboring bins — a window reduces this by tapering each block to zero at the edges. Second, stitching those processed blocks back together would create discontinuities — the 50% overlap and addition ensures each block connects smoothly to the next, but only if the overlapping windows sum to a constant, a requirement known as COLA (Constant Overlap Add). Each block is windowed twice, once before the FFT and once after the IFFT, so the code uses a square-root Hann window (`sin(πn/N)`): the two passes multiply into a periodic Hann window, which sums to exactly 1 at 50% overlap. A plain Hann window applied twice does not — its overlap-add gain wobbles between 0.5 and 1.0 every hop, which is audible as a 183 Hz buzz.

The ADC samples audio at 46.875kHz, triggered by TIM2. That odd number is deliberate: with the I2S PLL at 96MHz, the closest rate the I2S peripheral can produce to 48kHz is 46.875kHz (CubeMX shows the −2.34% error). TIM2 is set to 84MHz / 1792 to hit exactly the same rate, and since both clocks come from the same 8MHz crystal, input and output can never drift apart. With TIM2 at a true 48kHz, the ADC would outrun the I2S output and drop a block roughly four times a second. DMA handles the data movement in the background — without it the CPU would spend all its time copying samples and have nothing left for processing. Every 256 samples the main loop gets a flag and runs the FFT pipeline: convert ADC values to float, remove DC offset, apply the window, run a 512-point FFT, apply the spectral mask, IFFT back to time domain, window again, overlap-add with the previous block, then hand the result to I2S DMA for output.

Think of FFT as a döner slicer — it cuts the signal into individual frequency layers so you can manipulate each one independently. The luxury of processing every frequency comes with a time price: 512 samples at 46.875kHz means ~10.9ms of latency before you hear the result.

The CS43L22 codec takes the I2S stream and drives the headphone output. Its initialization is a sequence of register writes — some documented, some not — that have to happen in the right order or the chip stays silent.

## Configuring Filters

All filtering happens in `Init_Spectral_Mask()` in `audio_processing.c`. By default everything passes through. Uncomment the relevant block and adjust the frequency.

The bin number for any frequency is: `bin = frequency * FFT_SIZE / SAMPLE_RATE`, so at 46.875kHz with a 512-point FFT each bin is ~91.6Hz wide. The `(int)` cast rounds down, so 1kHz lands in bin 10 and 4kHz in bin 43.

**Notch filter** — remove a specific frequency (e.g. a 1kHz whine):
```c
float notch_freq = 1000.0f;
int notch_bin = (int)(notch_freq * FFT_SIZE / SAMPLE_RATE);
int notch_width = 2;  // widen if hum is broad
for (int k = notch_bin - notch_width; k <= notch_bin + notch_width; k++)
{
    if (k >= 0 && k <= FFT_SIZE / 2)
        spectral_mask[k] = 0.0f;
}
```

Measure your noise frequency first — a phone spectrum analyzer app works. Then calculate the bin, set the notch there, and widen `notch_width` if the noise is broad. Note that 50Hz mains hum sits inside bin 0 (DC) — at ~91.6Hz per bin the FFT is too coarse to notch it without also removing everything below ~230Hz, so a high-pass is the more honest tool for hum.

The other filter types (low-pass, high-pass, band-pass) follow the same pattern and are already in the code as commented blocks.

## Known Limitations

Analog noise from the Discovery board's shared 3.3V supply couples into the MAX4466 and shows up in the output. Shorter connections and a clean external supply help but the root cause is the board itself. This is a hardware problem — no amount of spectral filtering fully fixes a noisy input.

The MAX4466 is also not a high-performance microphone. It does the job for a prototype but don't expect clean audio at low signal levels.

## Building

```
make all
```

Do not regenerate from STM32CubeMX — it will overwrite the ADC sampling time fix in `MX_ADC1_Init` and revert it to the `.ioc` value.
