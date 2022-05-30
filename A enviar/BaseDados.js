var express = require('express');
var router = express.Router();
var db=require('./Database');
// another routes also appear here
// this script to fetch data from MySQL databse table

router.get('/',authenticationMiddleware() ,(req, res) => {
  
  var userData = [];
  var sql='SELECT * FROM tabeladb';
    db.query(sql, function (err, data, fields) {
    if (err) throw err;
    else 
      for (var i = 0; i< data.length; i++){
        if(i>=data.length-10){
        var dados = {
          'ID_SC':data[i].ID_SC,
          'ID':data[i].ID,
          'Timestamp':data[i].Timestamp,
          'Latitude':data[i].Latitude,
          'Longitude':data[i].Longitude,
          'Temperatura':data[i].Temperatura,
          'Humidade':data[i].Humidade,
          'Pressao':data[i].Pressão
        }
        console.log(dados);
        userData.push(dados);
      }
    }
      res.render("dados" ,{title: "Dados" ,"userData":userData});
});});

function authenticationMiddleware () {  
	return (req, res, next) => {
		console.log(`req.session.passport.user: ${JSON.stringify(req.session.passport)}`);

	    if (req.isAuthenticated()) return next();
	    res.redirect('/login')
	}
}
module.exports = router;
