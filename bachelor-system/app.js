const express = require("express");
const bodyParser = require("body-parser");
const mqtt = require("mqtt");
const robotRoutes = require("./routes/robotRoutes");
const sequelize = require("./database/db");

const RobotData = require("./models/robot");
const ApiDataDetail = require("./models/api");

const port = 3000;
const app = express();

app.set("view engine", "ejs");
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));
app.use(express.static("static"));

app.use(robotRoutes);

RobotData.hasMany(ApiDataDetail);
ApiDataDetail.belongsTo(RobotData);

sequelize
  .sync()
  .then((res) => {
    app.listen(port);
  })
  .catch((err) => console.log(err));

// const test = [
//   {
//     robot_id: "ROBOT-01",
//     upload_time: 1714287600,
//     location_lat: 56.9644612,
//     location_lng: 24.1108323,
//     location_elev: 1,
//     current_avg: 309,
//     voltage_avg: 1504,
//     power_avg: 0.464736,
//     battery_level: 100,
//     directNormalIrradience: 365,
//     cloudiness_prcntg: 32,
//   },
//   {
//     robot_id: "ROBOT-01",
//     upload_time: 1714287900,
//     location_lat: 56.9656701,
//     location_lng: 24.1094159,
//     location_elev: 1,
//     current_avg: 168,
//     voltage_avg: 4834,
//     power_avg: 0.812112,
//     battery_level: 100,
//     directNormalIrradience: 339,
//     cloudiness_prcntg: 36,
//   },
// ];

// const data = require("./static/data.json");
// data.data.forEach(async function (d) {
//   date = new Date(d.upload_time * 1000);
//   const robot = await RobotData.create({
//     robot_id: d.robot_id,
//     upload_time: date,
//     location_lat: d.location_lat,
//     location_lng: d.location_lng,
//     location_elev: d.location_elev,
//     current_avg: d.current_avg,
//     voltage_avg: d.voltage_avg,
//     power_avg: d.power_avg,
//     battery_level: d.battery_level,
//   });
//   const detail = await ApiDataDetail.create({
//     directNormalIrradiance: d.directNormalIrradience,
//     cloudiness_prcntg: d.cloudiness_prcntg,
//     robotDatumId: robot.id,
//   });
//   console.log("created objects");
// });
// const client = mqtt.connect("mqtt://10.10.10.175");

// client.on("connect", (conn) => {
//   console.log("Connected client!", conn);

//   client.subscribe("esp32/data");
// });

// client.on("message", (topic, message, packet) => {
//   console.log(topic);
//   console.log(JSON.parse(packet.payload.toString()));

//   client.publish(
//     "server/api-data",
//     JSON.stringify({ cloudy: 1, hz: "something more" })
//   );
// });

/*
(56,9658000, 24,1095000)         (56,9657429, 24,1118360)




(56,9642339, 24,1094112)         (56,9641637, 24,1116643)

*/
