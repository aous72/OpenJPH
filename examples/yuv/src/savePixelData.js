const fs = require('fs');

function writeToFile(outputFile, pixelData) {
  fs.writeFile(outputFile, pixelData, (err) => {
    if (err) {
      console.log(`unable to write pixel data to file ${outputFile}: ${err}`);
    } else {
      console.log(`successfully wrote pixel data to file ${outputFile}`);
    }
  });
}

module.exports = {
  writeToFile
}