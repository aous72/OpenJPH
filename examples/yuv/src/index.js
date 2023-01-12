const { generateYUVPixelData } = require('./yuv_generator');
const { writeToFile } = require('./savePixelData');
const { validateOptions } = require('./validateOptions');

(function main() {
  try {
    const options = process.argv.slice(2);
    if (options.length > 0 && options[0] === '-runtests') {
      require('./yuv_generator_tests');
    } else {
      const config = validateOptions(options);
      console.log(config);
      const pixelData = generateYUVPixelData(config);
      writeToFile(`samples\\${config.outputFile}`, pixelData);
    }
  } catch(e) {
    console.log(e.message);
  }
})();