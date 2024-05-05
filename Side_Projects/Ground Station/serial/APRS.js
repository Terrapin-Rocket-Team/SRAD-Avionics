/**
 * A class to convert APRS message output from the LoRa APRS library to a JSON object
 */
class APRSMessage {
  /**
   * @param {string|APRSMessage} message the APRS message
   */
  constructor(message) {
    if (typeof message === "object") {
      this.src = message.src;
      this.dest = message.dest;
      this.path = message.path;
      this.type = message.type;
      this.rssi = message.rssi;
      this.rawBody = message.rawBody;
      this.body = new APRSBody(message.body);
    }
    if (typeof message === "string") {
      this.src = message.match(/(?<=Source:)[^,]+(?=,)/g)[0];
      this.dest = message.match(/(?<=Destination:)[^,]+(?=,)/g)[0];
      this.path = message.match(/(?<=Path:)[^,]+(?=,)/g)[0];
      this.type = message.match(/(?<=Type:)[^,]+(?=,)/g)[0];
      this.rssi = message.match(/(?<=RSSI:).+$/g)[0];
      this.rawBody = message.match(/(?<=Data:).+(?=,(!w[^!]+!)?RSSI)/g)[0];
      this.body = new APRSBody(this.rawBody);
    }
  }

  /**
   * @returns {number[]} [latitude, longitude]
   */
  getLatLong() {
    return this.body.getLatLongDecimal();
  }

  /**
   * @param {Boolean} [dms] set true to format in degress, minutes, seconds
   * @returns {string} string containing the latitude and longitude
   */
  getLatLongFormat(dms) {
    if (!dms) return this.body.getLatLongDecimalFormatted();
    return this.body.getLatLongDMS();
  }

  /**
   * @returns {float} last updated altitude
   */
  getAlt() {
    return parseFloat(this.body.alt);
  }

  /**
   * @returns {Number} last updated heading
   */
  getHeading() {
    return parseInt(this.body.heading);
  }

  /**
   * @returns {Number} last updated speed
   */
  getSpeed() {
    return parseFloat(this.body.speed);
  }

  /**
   * @returns {String} the current stage in the form s(index), ex: s0
   */
  getStage() {
    return this.body.stage.toLowerCase();
  }

  /**
   * @returns {Number} the current stage number
   */
  getStageNumber() {
    return parseInt(this.body.stage.substring(1));
  }

  /**
   * @returns {Date} a Date object containing the date of T0
   */
  getT0() {
    return this.body.t0Date;
  }

  /**
   * @returns {Number} the T0 time in milliseconds
   */
  getT0ms() {
    return this.body.t0Date.getTime();
  }

  /**
   * @returns {String} High/Medium/Low/None signal strength, rssi range: >-60/-90<x<-60/-120<x<-90/<-120
   */
  getSignalStrength() {
    let rssi = parseInt(this.rssi);
    return rssi > -60
      ? "High"
      : rssi < -90 && rssi > -120
      ? "Low"
      : rssi > -90 && rssi < -60
      ? "Med"
      : "None";
  }

  /**
   * @returns {string} the APRS message object as a string
   */
  toString() {
    return `Source: ${this.src}, Dest: ${this.dest}, Path: ${
      this.path
    }, Type: ${this.type}, Body: ${this.body.toString()}, RSSI: ${this.rssi}`;
  }

  //convert lat/long to a better format
  toCSV(csvCreated) {
    let csv = "";
    if (!csvCreated) {
      csv =
        "Source,Destination,Path,Type,Raw Body,Latitude,Longitude,Heading,Speed,Altitude,Stage,T0,Signal Strength\r\n";
    }
    // console.log(this.rawBody);
    csv += `${this.src},${this.dest},${this.path},${this.type},${
      this.rawBody
    },${this.body.toCSV()},${this.rssi}\r\n`;
    return csv;
  }
}

/**
 * A class to convert the body of the APRS message
 */
class APRSBody {
  /**
   * @param {APRSBody|string} body the APRS body
   */
  constructor(body) {
    if (typeof body === "object") {
      this.lat = body.lat;
      this.long = body.long;
      this.heading = body.heading;
      this.speed = body.speed;
      this.alt = body.alt;
      this.stage = body.stage;
      this.t0 = body.t0;
      this.t0Date = this.dateFromT0(this.t0);
    }
    if (typeof body === "string") {
      this.lat = body.match(/(?<=!)[^\/]+(?=\/)/g)
        ? body.match(/(?<=!)[^\/]+(?=\/)/g)[0]
        : "";
      this.long = body.match(/(?<=\/)[^\[]+(?=\[)/g)
        ? body.match(/(?<=\/)[^\[]+(?=\[)/g)[0]
        : "";
      this.heading = body.match(/(?<=\[)[^\/]+(?=\/)/g)
        ? body.match(/(?<=\[)[^\/]+(?=\/)/g)[0]
        : "";
      this.speed = body.match(/(?<=\/)[^\/\[]+(?=\/)/g)
        ? body.match(/(?<=\/)[^\/\[]+(?=\/)/g)[0]
        : "";
      this.alt = body.match(/(?<=A=)-?[0-9]+/g)
        ? body.match(/(?<=A=)-?[0-9]+/g)[0]
        : "";
      this.stage = body.match(/(?<=\/)S[0-9]+(?=\/)/g)
        ? body.match(/(?<=\/)S[0-9]+(?=\/)/g)[0]
        : "";
      this.t0 = body.match(/(?<=\/)[0-9][0-9]:[0-9][0-9]:[0-9][0-9]/g)
        ? body.match(/(?<=\/)[0-9][0-9]:[0-9][0-9]:[0-9][0-9]/g)[0]
        : "";

      // only want to have to do this once
      this.t0Date = this.dateFromT0(this.t0);
    }
  }
  /**
   * @param {string} rawBody
   * @returns {object} object with the same structure as an APRS body
   */
  decodeBody(rawBody) {
    //based on a specific radio module library, will not work with other libraries
    let time0 = rawBody.match(/(?<=\/)[0-9][0-9]:[0-9][0-9]:[0-9][0-9]/g)
      ? rawBody.match(/(?<=\/)[0-9][0-9]:[0-9][0-9]:[0-9][0-9]/g)[0]
      : "";
    return {
      lat: rawBody.match(/(?<=!)[^\/]+(?=\/)/g)
        ? rawBody.match(/(?<=!)[^\/]+(?=\/)/g)[0]
        : "",
      long: rawBody.match(/(?<=\/)[^\[]+(?=\[)/g)
        ? rawBody.match(/(?<=\/)[^\[]+(?=\[)/g)[0]
        : "",
      heading: rawBody.match(/(?<=\[)[^\/]+(?=\/)/g)
        ? rawBody.match(/(?<=\[)[^\/]+(?=\/)/g)[0]
        : "",
      speed: rawBody.match(/(?<=\/)[^\/\[]+(?=\/)/g)
        ? rawBody.match(/(?<=\/)[^\/\[]+(?=\/)/g)[0]
        : "",
      alt: rawBody.match(/(?<=A=)-?[0-9]+/g)
        ? rawBody.match(/(?<=A=)-?[0-9]+/g)[0]
        : "",
      stage: rawBody.match(/(?<=\/)S[0-9]+(?=\/)/g)
        ? rawBody.match(/(?<=\/)S[0-9]+(?=\/)/g)[0]
        : "",
      t0: time0,
      t0Date: this.dateFromT0(time0),
    };
  }
  /**
   * @returns {string} the APRS body object as a string
   */
  toString() {
    return `${this.getLatLongDecimalFormatted()} and ${this.alt} at ${
      this.heading
    }\u00b0 ${this.speed} ft/s during stage ${this.stage.substring(
      1
    )}, T0 was at ${this.t0Date.toLocaleString()}`;
  }

  toCSV() {
    let ll = this.getLatLongDecimal();
    return `${ll[0]},${ll[1]},${this.heading},${this.speed},${this.alt},${this.stage},${this.t0}`;
  }

  toJSON() {
    // remove t0Date from JSON
    let result = {};
    for (let x in this) {
      if (x !== "t0Date") {
        result[x] = this[x];
      }
    }
    return result;
  }

  /**
   * Creates a Date object from a t0 time in UTC
   * @returns {Date} the Date object
   */
  dateFromT0(time0) {
    return new Date( // 2
      new Date()
        .toString()
        .match(
          /[A-Z][a-z][a-z] [A-Z][a-z][a-z] [0-9][0-9] [0-9][0-9][0-9][0-9] /g
        )[0] +
        new Date( // 1
          new Date()
            .toISOString()
            .match(
              /^([+-][0-9][0-9])?[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T/g
            )[0] +
            time0 +
            "Z"
        ).toTimeString() // 1 - get local t0
    ); // 2 get local date and combine with local t0
  }

  /**
   * @returns {Number[]} [latitude, longitude]
   */
  getLatLongDecimal() {
    return [
      this.getDegreesDecimal(this.lat),
      this.getDegreesDecimal(this.long),
    ];
  }

  /**
   * @returns {string} the latitude and longitude in decimal form formatted as a string
   */
  getLatLongDecimalFormatted() {
    return (
      this.getDegreesDecimal(this.lat, true).toFixed(4) +
      "\u00b0 " +
      this.lat.substring(this.lat.length - 1, this.lat.length) +
      "/" +
      this.getDegreesDecimal(this.long, true).toFixed(4) +
      "\u00b0 " +
      this.long.substring(this.long.length - 1, this.long.length)
    );
  }

  /**
   * @returns {string} the latitude and longitude in degress, minutes, seconds form as a string
   */
  getLatLongDMS() {
    return (
      this.getDegrees(this.lat) +
      "\u00b0" +
      this.getMinutes(this.lat) +
      "'" +
      this.getSeconds(this.lat) +
      '"' +
      this.lat.substring(this.lat.length - 1, this.lat.length) +
      " " +
      this.getDegrees(this.long) +
      "\u00b0" +
      this.getMinutes(this.long) +
      "'" +
      this.getSeconds(this.long) +
      '"' +
      this.long.substring(this.long.length - 1, this.long.length)
    );
  }
  /**
   * @param {string} coord string containing of the APRS formatted latitude or longitude
   * @param {boolean} [format] set to true to prevent use of negatives for West or South
   * @returns {number} latitude or longitude in the regular format
   */
  getDegreesDecimal(coord, format) {
    let dir = !format ? coord.substring(coord.length - 1, coord.length) : "";
    if (coord.length > 8) {
      return (
        (parseInt(coord.substring(0, 3)) +
          parseFloat(coord.substring(3, coord.length - 1)) / 60) *
        (dir === "S" || dir === "W" ? -1 : 1)
      );
    }
    return (
      (parseInt(coord.substring(0, 2)) +
        parseFloat(coord.substring(2, coord.length - 1)) / 60) *
      (dir === "S" || dir === "W" ? -1 : 1)
    );
  }

  /**
   * @param {string} coord string containing of the APRS formatted latitude or longitude
   * @returns {string} the degrees part of the coordinate
   */
  getDegrees(coord) {
    if (coord.length > 8) return coord.substring(0, 3);
    return coord.substring(0, 2);
  }

  /**
   * @param {string} coord string containing of the APRS formatted latitude or longitude
   * @returns {string} the minutes part of the coordinate
   */
  getMinutes(coord) {
    if (coord.length > 8) return coord.substring(3, 5);
    return coord.substring(2, 4);
  }

  /**
   * @param {string} coord string containing of the APRS formatted latitude or longitude
   * @returns {string} the seconds part of the coordinate
   */
  getSeconds(coord) {
    if (coord.length > 8) {
      return (parseFloat("0." + coord.substring(6, coord.length - 1)) * 60)
        .toFixed(0)
        .toString();
    }
    return (parseFloat("0." + coord.substring(5, coord.length - 1)) * 60)
      .toFixed(0)
      .toString();
  }
}

module.exports = { APRSMessage, APRSBody };
