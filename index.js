// test.js
var objectDetector = require('./build/Release/object-detector');

var obj = new objectDetector.Detector();
console.log( obj.detectInImageFile("test.jpg") ); // 11
