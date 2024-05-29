const Sequelize = require("sequelize");
const sequelize = require("../database/db");

const RobotData = sequelize.define("robot_data", {
  id: {
    type: Sequelize.INTEGER,
    allowNull: false,
    primaryKey: true,
    autoIncrement: true,
  },
  robot_id: {
    type: Sequelize.STRING,
  },
  upload_time: {
    type: Sequelize.DATE,
  },
  location_lng: {
    type: Sequelize.DECIMAL(10, 7),
  },
  location_lat: {
    type: Sequelize.DECIMAL(10, 7),
  },
  location_elev: {
    type: Sequelize.INTEGER,
  },
  current_avg: {
    type: Sequelize.INTEGER,
  },
  voltage_avg: {
    type: Sequelize.INTEGER,
  },
  power_avg: {
    type: Sequelize.DECIMAL(10, 7),
  },
  battery_level: {
    type: Sequelize.INTEGER,
  },
});
module.exports = RobotData;
