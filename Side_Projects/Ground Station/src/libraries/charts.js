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
  return new Chart(document.getElementById(id), {
    type: "line",
    data: {
      labels: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
      datasets: [
        {
          label: name,
          data: [],
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
          ticks: {
            callback: (value) =>
              `${
                (value * xConvert) % 1 > 0
                  ? (value * xConvert).toFixed(2)
                  : value * xConvert
              } ${xUnits}`,
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
  });
};
