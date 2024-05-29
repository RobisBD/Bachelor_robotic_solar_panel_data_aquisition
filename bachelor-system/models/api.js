const Sequelize = require("sequelize");
const sequelize = require("../database/db");

const ApiDataDetail = sequelize.define("api_data_detail", {
  id: {
    type: Sequelize.INTEGER,
    primaryKey: true,
    autoIncrement: true,
  },
  directNormalIrradiance: {
    type: Sequelize.DECIMAL,
  },
  cloudiness_prcntg: {
    type: Sequelize.INTEGER,
  },
});
module.exports = ApiDataDetail;
