/**
 *
 * @param {string} id the id of the HTML element for the chart
 * @param {string} name the name of the chart
 * @param {string} xUnits the units for the x axis
 * @param {string} yUnits the units for the y axis
 * @param {number} xConvert the conversion factor for the x axis
 * @param {number} yConvert the conversion factor for the y axis
 * @returns {Chart} the charts.js chart object
 */
const createChart = (id, name, xUnits, yUnits, xConvert, yConvert) => {
  return new Chart(
    document.getElementById(id),
    (ChartOptions = {
      type: "line",
      data: {
        labels: [0, 2, 4, 6, 8],
        datasets: [
          {
            label: name,
            data: [
              { x: 0, y: null },
              { x: 8, y: null },
            ],
            xAxisID: "x",
            yAxisID: "y",
          },
        ],
      },
      options: {
        aspectRatio: 1.6,
        maintainAspectRatio: false,
        animation: false,
        plugins: {
          legend: {
            display: false,
          },
          tooltip: {
            enabled: false,
          },
        },
        scales: {
          x: {
            type: "linear",
            afterTickToLabelConversion: function (axis) {
              axis.ticks.forEach((tick) => {
                tick.label = `${roundTo(tick.value * xConvert, 1)} ${xUnits}`;
              });
            },
            ticks: {
              count: 5,
            },
          },
          y: {
            beginAtZero: true,
            ticks: {
              callback: (value) => `${value * yConvert} ${yUnits}`,
            },
          },
        },
        elements: {
          point: {
            radius: 0,
          },
          line: {
            backgroundColor: "#000",
            borderColor: "#000",
          },
        },
      },
    })
  );
};

const roundTo = (n, d) => {
  return Math.round(n * Math.pow(10, d)) / Math.pow(10, d);
};
