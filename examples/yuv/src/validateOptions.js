const assert = require('assert');

/**
 * 
 * @param {Array} options
 */
function validateOptions(options) {
  const config = {};
  for (let j = 0; j < options.length; j+=2) {
    const optionCandidate = options[j];
    const optionValidator = option2ValidatorMap[optionCandidate];
    assert(optionValidator !== undefined, `unknown option ${optionCandidate} \n` + usage);
    assert(j+1 < options.length, `value not specified for option ${optionCandidate} \n` + usage);
    const optionValueCandidate = options[j+1];
    optionValidator(optionValueCandidate, config);
  }
  setDefaults(config);
  return config;
}

function setDefaults(config) {
  if (!config['width']) {
    config['width'] = 512;
  }
  if (!config['height']) {
    config['height'] = 512;
  }
  if (config['downsample_x'] === undefined) {
    config['downsample_x'] = false;
  }
  if (config['downsample_y'] === undefined) {
    config['downsample_y'] = false;
  }
  if (!config['bitdepth']) {
    config['bitdepth'] = 8;
  }
  if (config['signed'] === undefined) {
    config['signed'] = false;
  }
  if (config['isLittleEndian'] === undefined) {
    config['isLittleEndian'] = true;
  }
  if (config['interleaved'] === undefined) {
    config['interleaved'] = false;
  }
  if (config['random'] === undefined) {
    config['random'] = true;
  }
  config.is16Bit = (config.bitdepth > 8);
  config.minSampleValue = config.signed ? -Math.pow(2, config.bitdepth-1) : 0;
  config.maxSampleValue = config.signed ? Math.pow(2, config.bitdepth-1) - 1 : Math.pow(2, config.bitdepth)-1;
  config.outputFile = `sample_${config.width}_${config.height}_4${config.downsample_x ? (config.downsample_y ? "20" : "22") : "44"}_${config.signed ? "signed" : "unsigned"}_${config.bitdepth}bit_${config.isLittleEndian ? "LE" : "BE"}.yuv`;
}

const option2ValidatorMap = {
  
  '-dimension': function(value, config) {
    const format = /^\d+x\d+$/;
    const tokens = format.exec(value);
    assert(tokens !== null, `invalid dimension ${value} \n` + usage);
    assert(tokens.length === 1, `more than one match was found for dimension ${tokens.length}`)
    const token = tokens[0];
    const xIndex = token.indexOf('x');
    const width = Number(token.substring(0, xIndex));
    const height = Number(token.substring(xIndex+1));
    assert(width >= 64 && width <= 4096, `invalid width ${width} \n` + usage);
    assert(height >= 64 && height <= 4096, `invalid height ${height} \n` + usage);
    config['width'] = width;
    config['height'] = height;
  },
  
  '-downsample': function(value, config) {
    if (value === '4:4:4') {
      config['downsample_x'] = false;
      config['downsample_y'] = false;
    }
    if (value === '4:2:2') {
      assert(config['width'] !== undefined, `dimension must be specified before downsample \n` + usage);
      assert(config['width'] % 2 === 0, `invalid downsample ${value} \n` + usage);
      config['downsample_x'] = true;
      config['downsample_y'] = false;
    }
    if (value === '4:2:0') {
      assert(config['width'] !== undefined, `dimension must be specified before downsample \n` + usage);
      assert(config['height'] !== undefined, `dimension must be specified before downsample \n` + usage);
      assert(config['width'] % 2 === 0, `invalid downsample ${value} \n` + usage);
      assert(config['height'] % 2 === 0, `invalid downsample ${value} \n` + usage);
      config['downsample_x'] = true;
      config['downsample_y'] = true;
    }
  },

  '-bitdepth': function(value, config) {
    const format = /^[8-9]$|^1[0-5]$/;
    const tokens = format.exec(value);
    assert(tokens !== null, `invalid bitdepth ${value} \n` + usage);
    assert(tokens.length === 1, `more than one match was found for bitdepth ${tokens.length}`);
    const bitdepth = Number(tokens[0]);
    config['bitdepth'] = bitdepth;
  },

  '-endian': function(value, config) {
    const format = /^little$|^big$/;
    const tokens = format.exec(value);
    assert(tokens !== null, `invalid endian ${value} \n` + usage);
    assert(tokens.length === 1, `more than one match was found for endian ${tokens.length}`);
    const isLittleEndian = tokens[0] === 'little';
    assert(config.bitdepth !== undefined, `bitdepth must be specified before endian \n` + usage);
    assert(isLittleEndian || config.bitdepth > 8, `invalid endian ${value} \n` + usage);
    config['isLittleEndian'] = isLittleEndian;
  },

  '-signed': function(value, config) {
    const format = /^true$|^false$/;
    const tokens = format.exec(value);
    assert(tokens !== null, `invalid signed ${value} \n` + usage);
    assert(tokens.length === 1, `more than one match was found for signed ${tokens.length}`);
    const signed = tokens[0] === 'true';
    config['signed'] = signed;
  },

  '-interleaved': function(value, config) {
    const format = /^true$|^false$/;
    const tokens = format.exec(value);
    assert(tokens !== null, `invalid interleaved ${value} \n` + usage);
    assert(tokens.length === 1, `more than one match was found for interleaved ${tokens.length}`);
    const interleaved = tokens[0] === 'true';
    assert(config.downsample_x !== undefined, `downsample must be specified before interleaved \n` + usage);
    assert(!interleaved || (!config.downsample_x && !config.downsample_y), `invalid interleaved ${value} \n` + usage);
    config['interleaved'] = interleaved;
  },

  '-random': function(value, config) {
    const format = /^true$|^false$/;
    const tokens = format.exec(value);
    assert(tokens !== null, `invalid random ${value} \n` + usage);
    assert(tokens.length === 1, `more than one match was found for random ${tokens.length}`);
    const random = tokens[0] === 'true';
    config['random'] = random;
  },

};

const usage = `
Usage:

  node .\\src\\generator.js -dimension {width}x{height} -downsample {4:4:4 | 4:2:2 | 4:2:0} -bitdepth {depth} -endian {little | big} -signed {true | false} -interleaved {true | false} -random {true | false}

  width and height must be in the range [64, 4096]
  width must be even with 4:2:2 and 4:2:0
  height must be even with 4:2:0
  bitdepth must be an integer in the range [8, 15]
  endian must be little for bitdepth <= 8
  interleaved must be false when downsample is either 4:2:2 or 4:2:0
  if random is true, the samples are generated using a random number generator; if false, sample values are fixed
`;

module.exports = {
  validateOptions
};