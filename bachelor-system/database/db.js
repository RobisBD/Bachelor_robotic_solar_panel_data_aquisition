const Sequelize = require("sequelize");

const sequelize = new Sequelize("bachelor", "robis", "Rk#090300", {
  host: "localhost",
  dialect: "mysql",
});

module.exports = sequelize;
