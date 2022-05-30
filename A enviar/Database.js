//MYSQL
var mysql = require("mysql");

var connection = mysql.createConnection({
  host: "localhost",
  user: "root",
  port:3306,
  password: "Sins@741",
  database:'tabeladb'
})

connection.connect((err) =>{
  if(err){
    console.log("ERROR OCCURRED WITH CONNECTON ", err);
    res.status(404).json({ errors });
    return;
  }else{
    console.log("Conectado à base dados MySQL");
  }
});
module.exports =connection;
