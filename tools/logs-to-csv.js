#!/usr/bin/env node

/* global process */

var fs = require('fs');
var path = require('path');
var { queue } = require('d3-queue');
var oknok = require('oknok');

if (process.argv.length < 2) {
  console.error(
    'Usage: node logs-to-csv.js <directory with logs> > logs.csv'
  );
  process.exit(1);
}

const logsDir = process.argv[2];

var columnsByTypeAndName = {};
var q = queue();
var files = fs.readdirSync(logsDir);
files.filter(file => file.endsWith('.txt')).forEach(file => q.defer(processLogFile, file));
q.awaitAll(oknok({ ok: logTable, nok: logError }));

function processLogFile(file, done) {
  const filePath = path.join(logsDir, file);
  fs.readFile(filePath, { encoding: 'utf8' }, oknok({ nok: done, ok:addFileContentsToTable }));
  
  function addFileContentsToTable(contents) {
    const colName = path.basename(file, '.txt');
    const colType = colName.endsWith('-a') ? 'a' : 'b';
    var columnsByName = columnsByTypeAndName[colType];
    if (! columnsByName) {
      columnsByName = {};
      columnsByTypeAndName[colType] = columnsByName;
    }
    columnsByName[colName] = contents.split('\n');
    done();
  }
}

function logError(error) {
  console.error(error);
}

function logTable() {
  var aColumnNames = Object.keys(columnsByTypeAndName.a).sort();
  var bColumnNames = Object.keys(columnsByTypeAndName.b).sort();
  console.log(aColumnNames.join(',') + ',' + bColumnNames.join(','));

  var aColumns = aColumnNames.map(name => columnsByTypeAndName.a[name]);
  var bColumns = bColumnNames.map(name => columnsByTypeAndName.b[name]);

  for (let row = 0; aColumns.some(col => row < col.length || bColumns.some(col => row < col.length)); ++row) {
    const aVals = aColumns.map(col => row < col.length ? col[row] : '').join(',');
    const bVals = bColumns.map(col => row < col.length ? col[row] : '').join(',');
    console.log(aVals + ',' + bVals);
  }
}

