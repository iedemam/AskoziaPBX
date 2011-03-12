window.onload = checkAvailableWidth;
window.onresize = checkAvailableWidth;
function checkAvailableWidth(){
	var container = document.getElementById('allbody');
	container.style.width = (document.body.clientWidth > 900)? '900px' : 'auto';
}
