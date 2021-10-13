import handleError from 'handle-error-web';
import { version } from './package.json';
import { respondToFileChanges } from './responders/respond-to-file-changes';
import { graphArray } from './index';
import { select } from 'd3-selection';

(async function go() {
  window.onerror = reportTopLevelError;
  renderVersion();

  renderSource({ onArrays });

  function onArrays(namedArrays) {
    var containers = select('.result')
      .selectAll('.container')
      .data(namedArrays);
    containers.exit().remove();
    var newContainers = containers
      .enter()
      .append('div')
      .attr('class', 'container')
      .attr('id', (a, i) => 'container-' + i);
    newContainers.append('h3');

    newContainers
      .merge(containers)
      .each(callGraph)
      .select('h3')
      .text((na) => na.name);
  }

  function callGraph({ array }, i) {
    graphArray({
      id: i,
      array,
      containerSelector: '#' + this.getAttribute('id'),
      onError: handleError,
      fitToParentWidth: true,
      zoomable: true,
    });
  }

  function renderSource() {
    var inputSel = select('.input-source');
    inputSel.on('change', onMediaChange);

    function onMediaChange() {
      respondToFileChanges({ files: this.files, onArrays });
    }
  }
})();

function reportTopLevelError(msg, url, lineNo, columnNo, error) {
  handleError(error);
}

function renderVersion() {
  var versionInfo = document.getElementById('version-info');
  versionInfo.textContent = version;
}
