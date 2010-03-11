<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2008 IKT <http://itison-ikt.de>.
	All rights reserved.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
	AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
	OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

require_once("functions.inc");

/**
 * Returns all applications sorted by extension.
 * If no applications are defined, the global array is initialized.
 * This should be changed to return by reference to eliminate a lot of repetitive code.
 */
function applications_get_apps() {
	global $config;
	
	if (!is_array($config['dialplan']['application']))
		$config['dialplan']['application'] = array();
		
	applications_sort_apps();
		
	return $config['dialplan']['application'];
}

/**
 * Sorts applications by extension
 */
function applications_sort_apps() {
	global $config;

	usort($config['dialplan']['application'], "pbx_sort_by_extension");	
}

/**
 * Returns a specific application based on its uniqid
 */
function applications_get_app($uniqid) {
	global $config, $uniqid_map;

	// horribly inefficient but surely working replacement for broken uniqid_map index
	$apps = applications_get_apps();
	foreach ($apps as $app) {
		if ($app['uniqid'] == $uniqid) {
			return $app;
		}
	}
	/* $uniqid_map has regressed is unpredictable now that * and # characters are allowed in extensions
	It is probably not the culprit but rather the unpredictable things_get_thing() internal sorting behavior
	which should be removed so that sorting is done transparently
	return $config['dialplan']['application'][$uniqid_map[$uniqid]]; */
}

/**
 * Returns an array of all assigned application extensions
 */
function applications_get_extensions() {
	global $config;
	
	$extensions = array();

	if (is_array($config['dialplan']['application'])) {
		foreach($config['dialplan']['application'] as $app) {
			$extensions[] = $app['extension'];
		}
	}

	return $extensions;
}











function applications2_generate_default_application() {

	$defaultlogic = <<<EOD

<?php
  require('phpagi.php');

  \$agi = new AGI();

  \$agi->answer();
  \$agi->say_time();
  sleep(1);
  for($i = 0; $i < 5; $i++) {
    \$agi-say_digits($i);
  }
  \$agi->hangup();

?>

EOD;

	$application = array();

	$application['name'] = gettext("New Application");
	$application['uniqid'] = "DIALPLAN-APPLICATION-" . uniqid(rand());
	$application['applicationlogic'] = base64_encode($defaultlogic);
	$application['type'] = "php";

	return $application;
}

function applications2_get_applications() {

	$all_apps = $GLOBALS['config']['dialplan']['application2'];
	return is_array($all_apps) ? $all_apps : array();
}

function applications2_get_application($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['dialplan']['application2'][$uniqid_map[$uniqid]];
}

function applications2_verify_application($post, $errors) {

	$application = array();

	v_cleanliness(&$post, &$errors);
	v_required('name', gettext('Name'), &$application, &$post, &$errors);
	v_required('extension', gettext('Number'), &$application, &$post, &$errors);
	v_optional('descr', &$application, &$post);
	v_public_access_editor(&$application, &$post, &$errors);
	v_applicationlogic(&$application, &$post, &$errors);

	$trusted = array(
		'uniqid',
		'type'
	);
	foreach ($trusted as $t) {
		$application[$t] = $post[$t];
	}

	return $application;
}

function applications2_save_application($application) {
	global $g, $uniqid_map;

	if (applications2_get_application($application['uniqid'])) {
		$GLOBALS['config']['dialplan']['application2'][$uniqid_map[$application['uniqid']]] = $application;
	} else {
		$GLOBALS['config']['dialplan']['application2'][] = $application;
	}

	write_config();
	touch($g['dialplan_dirty_path']);
}

function applications2_delete_application($uniqid) {
	global $g, $uniqid_map;

	unset($GLOBALS['config']['dialplan']['application2'][$uniqid_map[$uniqid]]);

	write_config();
	touch($g['dialplan_dirty_path']);
}
?>
