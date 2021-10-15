import handleError from 'handle-error-web';

export function respondToFileChanges({ files, onArrays }) {
  var textPromises = Array.from(files).map((file) => file.text());
  Promise.all(textPromises).then(parse).catch(handleError);

  function parse(texts) {
    var arrays = texts.map((text, i) => ({
      name: files[i].name,
      array: text.split('\n').map((s) => +s),
    }));
    onArrays(arrays);
  }
}
