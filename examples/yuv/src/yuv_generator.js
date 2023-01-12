const assert = require('assert');

function generateRandomSample(config) {
  let value = Math.random();
  assert(value > 0.0 && value < 1.0, 'invalid sample');
  value = value * Math.pow(2, config.bitdepth);
  if (config.signed) {
    value -= Math.pow(2, config.bitdepth-1);
  }
  value = Math.floor(value);
  assert(value >= config.minSampleValue && value <= config.maxSampleValue, 'invalid sample');
  return value;
}

function generateFixedSample(config, plane) {
  if (plane === 'Y') {
    if (config.signed) {
      return -'Y'.charCodeAt(0);
    } else {
      return config.is16Bit ? 0x0100 + 'Y'.charCodeAt(0) : 'Y'.charCodeAt(0);
    }
  }
  if (plane === 'B') {
    if (config.signed) {
      return -'B'.charCodeAt(0);
    } else {
      return config.is16Bit ? 0x0100 + 'B'.charCodeAt(0) : 'B'.charCodeAt(0);
    }
  }
  if (plane === 'R') {
    if (config.signed) {
      return -'R'.charCodeAt(0);
    } else {
      return config.is16Bit ? 0x0100 + 'R'.charCodeAt(0) : 'R'.charCodeAt(0);
    }
  }
  throw Error(`unknown plane ${plane}`);
}

function generateYUVPixelData(config) {
  const lumaBufferSize = config.width * config.height * (config.is16Bit ? 2 : 1);
  const chromaBufferSize = (config.downsample_x && config.downsample_y) ? lumaBufferSize / 4 : ( config.downsample_x ? lumaBufferSize / 2 : lumaBufferSize );
  const bufferSize = lumaBufferSize + 2*chromaBufferSize;
  const buffer = new ArrayBuffer(bufferSize);
  let pixelData;
  if (config.signed) {
    pixelData = config.is16Bit ? new Int16Array(buffer) : new Int8Array(buffer);
  } else {
    pixelData = config.is16Bit ? new Uint16Array(buffer) : new Uint8Array(buffer);
  }
  if (config.interleaved) {
    let index = 0;
    const numberOfPixelsOnLumaBuffer = lumaBufferSize / (config.is16Bit ? 2 : 1);
    for (let j = 0; j < numberOfPixelsOnLumaBuffer; j++) {
      const YSample = config.random ? generateRandomSample(config) : generateFixedSample(config, 'Y');
      const BSample = config.random ? generateRandomSample(config) : generateFixedSample(config, 'B');
      const RSample = config.random ? generateRandomSample(config) : generateFixedSample(config, 'R');
      assert(index+2 < pixelData.length, `array index out of bound: index=${index+2}, pixel data size=${pixelData.length}`);
      pixelData[index++] = adjustSampleForWritingInLittleEndianBuffer(adjustSampleToBitDepth(YSample, config), config);
      pixelData[index++] = adjustSampleForWritingInLittleEndianBuffer(adjustSampleToBitDepth(BSample, config), config);
      pixelData[index++] = adjustSampleForWritingInLittleEndianBuffer(adjustSampleToBitDepth(RSample, config), config);
    }
  } else {
    let index = 0;
    const numberOfPixelsOnLumaPlane = lumaBufferSize / (config.is16Bit ? 2 : 1);
    const numberOfPixelsOnChromaPlane = chromaBufferSize / (config.is16Bit ? 2 : 1);
    for (let j = 0; j < numberOfPixelsOnLumaPlane; j++) {
      const nextSample = config.random ? generateRandomSample(config) : generateFixedSample(config, 'Y');
      assert(index < pixelData.length, `array index out of bound: index=${index}, pixel data size=${pixelData.length}`);
      pixelData[index++] = adjustSampleForWritingInLittleEndianBuffer(adjustSampleToBitDepth(nextSample, config), config);
    }
    for (let j = 0; j < numberOfPixelsOnChromaPlane; j++) {
      const nextSample = config.random ? generateRandomSample(config) : generateFixedSample(config, 'B');
      assert(index < pixelData.length, `array index out of bound: index=${index}, pixel data size=${pixelData.length}`);
      pixelData[index++] = adjustSampleForWritingInLittleEndianBuffer(adjustSampleToBitDepth(nextSample, config), config);
    }
    for (let j = 0; j < numberOfPixelsOnChromaPlane; j++) {
      const nextSample = config.random ? generateRandomSample(config) : generateFixedSample(config, 'R');
      assert(index < pixelData.length, `array index out of bound: index=${index}, pixel data size=${pixelData.length}`);
      pixelData[index++] = adjustSampleForWritingInLittleEndianBuffer(adjustSampleToBitDepth(nextSample, config), config);
    }
  }
  return pixelData;
}

function adjustSampleToBitDepth(sample, config) {
  const factor = Math.pow(2, config.bitdepth)-1;
  return sample & factor;
}

function adjustSampleForWritingInLittleEndianBuffer(sample, config) {
  if (config.is16Bit && !config.isLittleEndian) {
    return ((sample & 0xFF) << 8) + ((sample & 0xFF00) >> 8);
  }
  return sample;
}

module.exports = {
  generateYUVPixelData
}