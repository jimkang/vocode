#!/usr/bin/env node

/* global process */

var fs = require("fs");
var imgFileExts = ["png", "jpg"];

if (process.argv.length < 5) {
  console.error(
    "Usage: node tools/wrap-images-in-html.js <directory with images> <path to page template> <path to entry template>"
  );
  process.exit(1);
}

const imgDirPath = process.argv[2];
var pageTemplate = fs.readFileSync(__dirname + "/../" + process.argv[3], {
  encoding: "utf8",
});
var entryTemplate = fs.readFileSync(__dirname + "/../" + process.argv[4], {
  encoding: "utf8",
});

var files = fs.readdirSync(imgDirPath).filter(probablyAnImageFile);

var entriesHTML = files.map(makeHTMLForEntry).join("\n");
console.log(pageTemplate.replace(/__ENTRIES__/, entriesHTML));

function makeHTMLForEntry(file) {
  var html = entryTemplate.slice();
  var entry = {
    name: file,
    imgurl: file,
    imgalt: file,
  };
  for (var key in entry) {
    let placeholderRegex = new RegExp(`__${key.toUpperCase()}__`, "g");
    html = html.replace(placeholderRegex, entry[key]);
  }
  return html;
}

function logError(error) {
  console.log(error);
}

function probablyAnImageFile(file) {
  return imgFileExts.some((ext) => file.endsWith("." + ext));
}
