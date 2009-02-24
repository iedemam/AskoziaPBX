
jQuery.noConflict();

jQuery(document).ready(function(){

	jQuery.preloadImages(['/tri_o.gif', '/tri_c.gif']);

});


function showhide(tspan, tri) {
	tspanel = document.getElementById(tspan);
	triel = document.getElementById(tri);
	if (tspanel.style.display == 'none') {
		jQuery("#" + tspan).slideDown("slow");
		triel.src = "tri_o.gif";
	} else {
		jQuery("#" + tspan).slideUp("slow");
		triel.src = "tri_c.gif";
	}
}
