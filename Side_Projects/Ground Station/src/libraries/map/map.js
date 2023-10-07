let map;
const markers = new L.featureGroup();

/**
 * @param {string} id the id of the HTML element for the map
 */
const buildMap = (id) => {
  //create the map and set defaults
  map = new L.Map(id, {
    //38.987810, -76.942406
    center: new L.LatLng(38.98781, -76.942406),
    maxZoom: 15,
    zoom: 5,
  });

  L.tileLayer.provider("OpenTopoMap").addTo(map);

  markers.addTo(map);

  let marker = L.marker([38.98781, -76.942406]);

  marker.addTo(markers);
  map.fitBounds(markers.getBounds());
  map.setZoom(12);
  markers.clearLayers();

  L.easyButton(
    '<img src="../src/images/home_map.svg" style="width:100%;height;100%;">',
    (btn, map) => {
      map.setView([38.98781, -76.942406]);
      map.setZoom(15);
    }
  ).addTo(map);
};

/**
 * @param {number|string} lat the latitude for the marker
 * @param {number|string} lng the longitude for the marker
 * @param {HTMLElement} html the html for the popup
 */
const updateMarker = (lat, lng, html) => {
  markers.clearLayers();
  let marker = L.marker([lat, lng]);
  marker.bindTooltip(html, { permanent: true }).openTooltip();
  marker.addTo(markers);
  map.fitBounds(markers.getBounds());
  map.setZoom(14);
  setTimeout(() => {
    map.setZoom(15);
  }, 2000);
};

//clear markers and refresh map sizing, needs to be called when a page is loaded or the tiles will not load correctly
const refreshMap = (zoom) => {
  //markers.clearLayers();
  setTimeout(() => {
    map.invalidateSize();
    map.setZoom(zoom == undefined ? 5 : zoom);
  }, 1000);
};
