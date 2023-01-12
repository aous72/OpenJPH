const { generateYUVPixelData } = require("./yuv_generator");
const { validateOptions } = require("./validateOptions");
const assert = require('assert');

(function runAllTests() {
  test___signed__8bit_LE_sequential_fixed_downsampleX();
  test___signed_16bit_LE_sequential_fixed_downsampleX();
  test___signed_16bit_BE_sequential_fixed_downsampleX();
  test_unsigned__8bit_LE_sequential_fixed_downsampleX();
  test_unsigned_16bit_LE_sequential_fixed_downsampleX();
  test_unsigned_16bit_BE_sequential_fixed_downsampleX();
  test_unsigned__8bit_LE_sequential_fixed_downsampleXY();
  test_unsigned_16bit_LE_sequential_fixed_downsampleXY();
  test_unsigned_16bit_BE_sequential_fixed_downsampleXY();
  test___signed_16bit_LE_interleave_fixed();
  test___signed_16bit_BE_interleave_fixed();
  test_unsigned__8bit_LE_interleave_fixed();
  test_unsigned_16bit_LE_interleave_fixed();
  test_unsigned_16bit_BE_interleave_fixed();
})();

function test___signed__8bit_LE_sequential_fixed_downsampleX() {
  const options = ['-dimension', '64x64', '-downsample', '4:2:2', '-signed', 'true', '-bitdepth', '8', '-endian', 'little', '-interleaved', 'false', '-random', 'false'];
  const config = validateOptions(options);
  const buffer = generateYUVPixelData(config).buffer;
  const pixelData = new Uint8Array(buffer);
  assert(pixelData.length === 8192, `failed: ${arguments.callee.name}   cause: wrong pixel data size`);
  let index = 0;
  let target = (-'Y'.charCodeAt(0)) & 0xFF;
  for (let column = 0; column < 64; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === target, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  target = (-'B'.charCodeAt(0)) & 0xFF;
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === target, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  target = (-'R'.charCodeAt(0)) & 0xFF;
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === target, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  assert(pixelData.length === index, `failed: ${arguments.callee.name}   cause: wrong index`);
  console.log(`passed: ${arguments.callee.name}`);
}

function test___signed_16bit_LE_sequential_fixed_downsampleX() {
  const options = ['-dimension', '64x64', '-downsample', '4:2:2', '-signed', 'true', '-bitdepth', '12', '-endian', 'little', '-interleaved', 'false', '-random', 'false'];
  const config = validateOptions(options);
  const buffer = generateYUVPixelData(config).buffer;
  const pixelData = new Uint8Array(buffer);
  assert(pixelData.length === 16384, `failed: ${arguments.callee.name}   cause: wrong pixel data size`);
  let index = 0;
  let data = (-'Y'.charCodeAt(0)) & 0xFF;
  for (let column = 0; column < 64; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === data, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 0x0F, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  data = (-'B'.charCodeAt(0)) & 0xFF;
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === data, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 0x0F, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  data = (-'R'.charCodeAt(0)) & 0xFF;
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === data, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 0x0F, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  assert(pixelData.length === index, `failed: ${arguments.callee.name}   cause: wrong index`);
  console.log(`passed: ${arguments.callee.name}`);
}

function test___signed_16bit_BE_sequential_fixed_downsampleX() {
  const options = ['-dimension', '64x64', '-downsample', '4:2:2', '-signed', 'true', '-bitdepth', '12', '-endian', 'big', '-interleaved', 'false', '-random', 'false'];
  const config = validateOptions(options);
  const buffer = generateYUVPixelData(config).buffer;
  const pixelData = new Uint8Array(buffer);
  assert(pixelData.length === 16384, `failed: ${arguments.callee.name}   cause: wrong pixel data size`);
  let index = 0;
  let data = (-'Y'.charCodeAt(0)) & 0xFF;
  for (let column = 0; column < 64; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === 0x0F, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === data, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  data = (-'B'.charCodeAt(0)) & 0xFF;
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === 0x0F, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === data, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  data = (-'R'.charCodeAt(0)) & 0xFF;
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === 0x0F, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === data, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  assert(pixelData.length === index, `failed: ${arguments.callee.name}   cause: wrong index`);
  console.log(`passed: ${arguments.callee.name}`);
}

function test_unsigned__8bit_LE_sequential_fixed_downsampleX() {
  const options = ['-dimension', '64x64', '-downsample', '4:2:2', '-signed', 'false', '-bitdepth', '8', '-endian', 'little', '-interleaved', 'false', '-random', 'false'];
  const config = validateOptions(options);
  const buffer = generateYUVPixelData(config).buffer;
  const pixelData = new Uint8Array(buffer);
  assert(pixelData.length === 8192, `failed: ${arguments.callee.name}   cause: wrong pixel data size`);
  let index = 0;
  for (let column = 0; column < 64; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === 'Y'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === 'B'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === 'R'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  assert(pixelData.length === index, `failed: ${arguments.callee.name}   cause: wrong index`);
  console.log(`passed: ${arguments.callee.name}`);
}

function test_unsigned_16bit_LE_sequential_fixed_downsampleX() {
  const options = ['-dimension', '64x64', '-downsample', '4:2:2', '-signed', 'false', '-bitdepth', '12', '-endian', 'little', '-interleaved', 'false', '-random', 'false'];
  const config = validateOptions(options);
  const buffer = generateYUVPixelData(config).buffer;
  const pixelData = new Uint8Array(buffer);
  assert(pixelData.length === 16384, `failed: ${arguments.callee.name}   cause: wrong pixel data size`);
  let index = 0;
  for (let column = 0; column < 64; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === 'Y'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === 'B'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === 'R'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  assert(pixelData.length === index, `failed: ${arguments.callee.name}   cause: wrong index`);
  console.log(`passed: ${arguments.callee.name}`);
}

function test_unsigned_16bit_BE_sequential_fixed_downsampleX() {
  const options = ['-dimension', '64x64', '-downsample', '4:2:2', '-signed', 'false', '-bitdepth', '12', '-endian', 'big', '-interleaved', 'false', '-random', 'false'];
  const config = validateOptions(options);
  const buffer = generateYUVPixelData(config).buffer;
  const pixelData = new Uint8Array(buffer);
  assert(pixelData.length === 16384, `failed: ${arguments.callee.name}   cause: wrong pixel data size`);
  let index = 0;
  for (let column = 0; column < 64; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 'Y'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 'B'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 'R'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  assert(pixelData.length === index, `failed: ${arguments.callee.name}   cause: wrong index`);
  console.log(`passed: ${arguments.callee.name}`);
}

function test_unsigned__8bit_LE_sequential_fixed_downsampleXY() {
  const options = ['-dimension', '64x64', '-downsample', '4:2:0', '-signed', 'false', '-bitdepth', '8', '-endian', 'little', '-interleaved', 'false', '-random', 'false'];
  const config = validateOptions(options);
  const buffer = generateYUVPixelData(config).buffer;
  const pixelData = new Uint8Array(buffer);
  assert(pixelData.length === 6144, `failed: ${arguments.callee.name}   cause: wrong pixel data size`);
  let index = 0;
  for (let column = 0; column < 64; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === 'Y'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 32; row++) {
      assert(pixelData[index++] === 'B'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 32; row++) {
      assert(pixelData[index++] === 'R'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  assert(pixelData.length === index, `failed: ${arguments.callee.name}   cause: wrong index`);
  console.log(`passed: ${arguments.callee.name}`);
}

function test_unsigned_16bit_LE_sequential_fixed_downsampleXY() {
  const options = ['-dimension', '64x64', '-downsample', '4:2:0', '-signed', 'false', '-bitdepth', '12', '-endian', 'little', '-interleaved', 'false', '-random', 'false'];
  const config = validateOptions(options);
  const buffer = generateYUVPixelData(config).buffer;
  const pixelData = new Uint8Array(buffer);
  assert(pixelData.length === 12288, `failed: ${arguments.callee.name}   cause: wrong pixel data size`);
  let index = 0;
  for (let column = 0; column < 64; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === 'Y'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 32; row++) {
      assert(pixelData[index++] === 'B'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 32; row++) {
      assert(pixelData[index++] === 'R'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  assert(pixelData.length === index, `failed: ${arguments.callee.name}   cause: wrong index`);
  console.log(`passed: ${arguments.callee.name}`);
}

function test_unsigned_16bit_BE_sequential_fixed_downsampleXY() {
  const options = ['-dimension', '64x64', '-downsample', '4:2:0', '-signed', 'false', '-bitdepth', '12', '-endian', 'big', '-interleaved', 'false', '-random', 'false'];
  const config = validateOptions(options);
  const buffer = generateYUVPixelData(config).buffer;
  const pixelData = new Uint8Array(buffer);
  assert(pixelData.length === 12288, `failed: ${arguments.callee.name}   cause: wrong pixel data size`);
  let index = 0;
  for (let column = 0; column < 64; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 'Y'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 32; row++) {
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 'B'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  for (let column = 0; column < 32; column++) {
    for (let row = 0; row < 32; row++) {
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 'R'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  assert(pixelData.length === index, `failed: ${arguments.callee.name}   cause: wrong index`);
  console.log(`passed: ${arguments.callee.name}`);
}

function test___signed_16bit_LE_interleave_fixed() {
  const options = ['-dimension', '64x64', '-downsample', '4:4:4', '-signed', 'true', '-bitdepth', '12', '-endian', 'little', '-interleaved', 'true', '-random', 'false'];
  const config = validateOptions(options);
  const buffer = generateYUVPixelData(config).buffer;
  const pixelData = new Uint8Array(buffer);
  assert(pixelData.length === 24576, `failed: ${arguments.callee.name}   cause: wrong pixel data size`);
  let index = 0;
  const data1 = (-'Y'.charCodeAt(0)) & 0xFF;
  const data2 = (-'B'.charCodeAt(0)) & 0xFF;
  const data3 = (-'R'.charCodeAt(0)) & 0xFF;
  for (let column = 0; column < 64; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === data1, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 0x00F, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === data2, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 0x00F, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === data3, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 0x00F, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  assert(pixelData.length === index, `failed: ${arguments.callee.name}   cause: wrong index`);
  console.log(`passed: ${arguments.callee.name}`);
}

function test___signed_16bit_BE_interleave_fixed() {
  const options = ['-dimension', '64x64', '-downsample', '4:4:4', '-signed', 'true', '-bitdepth', '12', '-endian', 'big', '-interleaved', 'true', '-random', 'false'];
  const config = validateOptions(options);
  const buffer = generateYUVPixelData(config).buffer;
  const pixelData = new Uint8Array(buffer);
  assert(pixelData.length === 24576, `failed: ${arguments.callee.name}   cause: wrong pixel data size`);
  let index = 0;
  const data1 = (-'Y'.charCodeAt(0)) & 0xFF;
  const data2 = (-'B'.charCodeAt(0)) & 0xFF;
  const data3 = (-'R'.charCodeAt(0)) & 0xFF;
  for (let column = 0; column < 64; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === 0x00F, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === data1, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 0x00F, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === data2, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 0x00F, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === data3, `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  assert(pixelData.length === index, `failed: ${arguments.callee.name}   cause: wrong index`);
  console.log(`passed: ${arguments.callee.name}`);
}

function test_unsigned__8bit_LE_interleave_fixed() {
  const options = ['-dimension', '64x64', '-downsample', '4:4:4', '-signed', 'false', '-bitdepth', '8', '-endian', 'little', '-interleaved', 'true', '-random', 'false'];
  const config = validateOptions(options);
  const buffer = generateYUVPixelData(config).buffer;
  const pixelData = new Uint8Array(buffer);
  assert(pixelData.length === 12288, `failed: ${arguments.callee.name}   cause: wrong pixel data size`);
  let index = 0;
  for (let column = 0; column < 64; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === 'Y'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 'B'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 'R'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  assert(pixelData.length === index, `failed: ${arguments.callee.name}   cause: wrong index`);
  console.log(`passed: ${arguments.callee.name}`);
}

function test_unsigned_16bit_LE_interleave_fixed() {
  const options = ['-dimension', '64x64', '-downsample', '4:4:4', '-signed', 'false', '-bitdepth', '12', '-endian', 'little', '-interleaved', 'true', '-random', 'false'];
  const config = validateOptions(options);
  const buffer = generateYUVPixelData(config).buffer;
  const pixelData = new Uint8Array(buffer);
  assert(pixelData.length === 24576, `failed: ${arguments.callee.name}   cause: wrong pixel data size`);
  let index = 0;
  for (let column = 0; column < 64; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] === 'Y'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 'B'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 'R'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  assert(pixelData.length === index, `failed: ${arguments.callee.name}   cause: wrong index`);
  console.log(`passed: ${arguments.callee.name}`);
}

function test_unsigned_16bit_BE_interleave_fixed() {
  const options = ['-dimension', '64x64', '-downsample', '4:4:4', '-signed', 'false', '-bitdepth', '12', '-endian', 'big', '-interleaved', 'true', '-random', 'false'];
  const config = validateOptions(options);
  const buffer = generateYUVPixelData(config).buffer;
  const pixelData = new Uint8Array(buffer);
  assert(pixelData.length === 24576, `failed: ${arguments.callee.name}   cause: wrong pixel data size`);
  let index = 0;
  for (let column = 0; column < 64; column++) {
    for (let row = 0; row < 64; row++) {
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 'Y'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 'B'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] ===  1               , `failed: ${arguments.callee.name}   cause: wrong pixel value`);
      assert(pixelData[index++] === 'R'.charCodeAt(0), `failed: ${arguments.callee.name}   cause: wrong pixel value`);
    }
  }
  assert(pixelData.length === index, `failed: ${arguments.callee.name}   cause: wrong index`);
  console.log(`passed: ${arguments.callee.name}`);
}