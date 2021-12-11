import { select } from 'd3-selection';
import { scaleLinear } from 'd3-scale';
import curry from 'lodash.curry';
import { zoom as Zoom } from 'd3-zoom';

// Returns yBounds in object.
export async function graphArray({
  id,
  array,
  containerSelector,
  color = 'hsl(80, 50%, 60%)',
  waveformWidth = 800,
  waveformHeight = 300,
  fitToParentWidth = false,
  zoomable = false,
  yBounds,
  renderInfinities = true,
  maxMagToShow
}) {
  var nonInfArray = array;
  var infArray = [];
  if (renderInfinities) {
    nonInfArray = [];
    for (let i = 0; i < array.length; ++i) {
      const val = array[i];
      if (val === Infinity) {
        infArray[i] = yBounds[1];
        nonInfArray[i] = 0;
      } else if (val === -Infinity) {
        infArray[i] = yBounds[0];
        nonInfArray[i] = 0;
      } else {
        infArray[i] = 0;
        nonInfArray[i] = val;
      }
    }
  }      
  if (!isNaN(maxMagToShow)) {
    nonInfArray = nonInfArray.map(x => Math.abs(x) >= maxMagToShow ? 0 : x);
  }

  var width = waveformWidth;
  if (fitToParentWidth) {
    width = document.body.getBoundingClientRect().width;
  }

  var containerSel = select(containerSelector);
  containerSel.selectAll('canvas').style('display', 'none');
  renderArray();
  containerSel.classed('hidden', false);

  function initCanvas(canvasClass, width, sel) {
    sel.classed(canvasClass, true).classed('waveform', true).attr('width', width).attr('height', waveformHeight);
  }

  function renderArray() {
    var currentTransform = Zoom.zoomIdentity;
    const canvasClass = `waveform-${id}`;

    var canvasSel = establish({
      parentSel: containerSel,
      childTag: 'canvas',
      childSelector: '.' + canvasClass,
      initFn: curry(initCanvas)(canvasClass, width),
    });
    canvasSel.style('display', 'block');
    const height = canvasSel.attr('height');

    if (zoomable) {
      setUpZoom(canvasSel.node(), draw);
    }

    var canvasCtx = canvasSel.node().getContext('2d', { alpha: false });
    canvasCtx.lineWidth = 1;
    canvasCtx.fillStyle = '#333';

    draw();

    function draw() {
      drawWaveform({
        canvasSel,
        array: nonInfArray,
        color,
        transform: currentTransform
      });
      drawWaveform({
        clearBeforeDraw: false,
        canvasSel,
        array: infArray,
        color: 'hsla(0, 0%, 100%, 0.2)',
        transform: currentTransform
      });
    }

    function setUpZoom(canvas, draw, initialTransform = undefined) {
      var zoom = Zoom()
        .scaleExtent([1, 256])
        .on('zoom', zoomed);

      var canvasSel = select(canvas);
      canvasSel.call(zoom);

      if (initialTransform) {
        canvasSel.call(zoom.transform, initialTransform);
      }

      function zoomed(zoomEvent) {
        currentTransform = zoomEvent.transform;
        draw(currentTransform);
      }
    }

    function drawWaveform({ array, color, transform, clearBeforeDraw = true }) {
      if (clearBeforeDraw) {
        canvasCtx.clearRect(0, 0, width, height);
        canvasCtx.fillRect(0, 0, width, height);
      }

      var x = scaleLinear().domain([0, array.length]).range([0, width]);
      // In canvas, and GUIs in general, remember:
      // +y is down! If we want positive values to be
      // higher than negative ones, we must flip their
      // signs.
      var y = scaleLinear().domain(yBounds).range([height, 0]);
      canvasCtx.beginPath();
      canvasCtx.strokeStyle = color;
      if (transform) {
        canvasCtx.moveTo(0, transform.applyY(y(0)));
      } else {
        canvasCtx.moveTo(0, y(0));
      }
      for (let i = 0; i < array.length; ++i) {
        const val = array[i];
        let yPos = y(val);
        let xPos = x(i);
        if (transform) {
          yPos = transform.applyY(yPos);
          xPos = transform.applyX(xPos);
        }
        canvasCtx.lineTo(xPos, yPos);
      }
      canvasCtx.stroke();
    }
  }
}

// parentSel should be a d3 selection.
export function establish({
  parentSel,
  childTag,
  childSelector,
  initFn
}) {
  var childSel = parentSel.select(childSelector);
  if (childSel.empty()) {
    childSel = parentSel.append(childTag);
    if (initFn) {
      initFn(childSel);
    }
  }
  return childSel;
}
