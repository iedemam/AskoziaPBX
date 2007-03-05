<?
/*
	auth.inc
	part of m0n0wall (http://m0n0.ch/wall)

	Copyright (C) 2006 Paul Taylor <paultaylor@winn-dixie.com>.
	All rights reserved. 
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
	All rights reserved.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
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

function index_groups() {
    global $g, $config;
    
    if (isset($config['system']['group'])) {
        $i = 0; 
	    foreach($config['system']['group'] as $groupent) {
	        $groupindex[$groupent['name']] = $i;
	        $i++;
	    }
	}
	return ($groupindex);
}

function index_users() {
    global $g, $config;
    
    if (isset($config['system']['user'])) {
        $i = 0; 
    	foreach($config['system']['user'] as $userent) {
	       $userindex[$userent['name']] = $i;
	       $i++;
	   }
    }
    return ($userindex);
}

$groupindex = index_groups();
$userindex = index_users();

// Once here, the user has authenticated with the web server.  
// Now, we give them access only to the appropriate pages for their group.
if (!($_SERVER['REMOTE_USER'] === $config['system']['username'])) {
	$allowed[] = '';
	if (isset($config['system']['group'][$groupindex[$config['system']['user'][$userindex[$_SERVER['REMOTE_USER']]]['groupname']]]['pages'])) {
        $allowed = &$config['system']['group'][$groupindex[$config['system']['user'][$userindex[$_SERVER['REMOTE_USER']]]['groupname']]]['pages'];
	} 

	// If the user is attempting to hit the default page, set it to specifically look for /index.php.
	// Without this, any user would have access to the index page.
	if ($_SERVER['PHP_SELF'] == '/') 
		$_SERVER['PHP_SELF'] = '/index.php';
	
	// Strip the leading / from the currently requested PHP page
	// With extensions there also is a path to be concerned with
    if ( !in_array(ltrim($_SERVER['PHP_SELF'],"/"),$allowed) ) {
		// The currently logged in user is not allowed to access the page
		// they are attempting to go to.  Redirect them to an allowed page.

		if (in_array("index.php",$allowed)) {
			header("Location: /index.php"); 
			exit;
		}
		else {
			header("HTTP/1.0 401 Unauthorized");
			header("Status: 401 Unauthorized");

			echo "<html><head><title>401 Unauthorized</title></head><body><h3>401 Unauthorized</h3>Authorization required.</body></html>";
			exit;
		}
	}
}


?>