var db = require('./Database');

var login =function(user,password){
    console.log(user,password)
    db.query('SELECT * FROM Users WHERE e-mail = user AND  =', function (err, data, fields) {
    }
)};
module.exports=login;