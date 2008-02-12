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

require_once("functions.inc");

/**
 * Voicemail notification e-mail subject lines
 * XXX : consolidate in help.inc or something like languages.inc
 */
$vm_emailsubject = array(

	// danish
	"da-informal" => 	"Ny telefonsvarer besked (boks: $\{VM_MAILBOX})",
	"da-formal" => 		"Ny telefonsvarer besked (boks: $\{VM_MAILBOX})",

	// german
	"de-informal" => 	"Neue Sprachmitteilung (Konto: $\{VM_MAILBOX})",
	"de-formal" => 		"Neue Sprachmitteilung (Konto: $\{VM_MAILBOX})",
	
	// english
	"en-informal" => 	"New voicemail message (box: $\{VM_MAILBOX})",
	"en-formal" => 		"New voicemail message (box: $\{VM_MAILBOX})",

	// spanish
	"es-formal" => 		"Nuevo mensaje de voz (cuenta: $\{VM_MAILBOX})",

	// french
	"fr-informal" => 	"tu as reçu un nouveau message dans ta boite vocale",
	"fr-formal" => 		"vous avez reçu un nouveau message dans votre boite vocale",

	// italian
	"it-formal" => 		"Nuovo messaggio voicemail (box: $\{VM_MAILBOX})",
	"it-informal" =>	"Nuovo messaggio voicemail per te (box: $\{VM_MAILBOX})",

	// dutch
	"nl-informal" => 	"Nieuw voicemail bericht (extensie: $\{VM_MAILBOX})",
	"nl-formal" => 		"Nieuw voicemail bericht (extensie: $\{VM_MAILBOX})",

	// polish
	"pl-informal" => 	"Nowa wiadomość głosowa (box: $\{VM_MAILBOX})",
	"pl-formal" => 		"Nowa wiadomość głosowa (box: $\{VM_MAILBOX})",

	// portuguese, brazil
	"pt-br-informal" => "Nova mensagem de voz (caixa: $\{VM_MAILBOX})",
	"pt-br-formal" => 	"Nova mensagem de voz (caixa: $\{VM_MAILBOX})",

	// russian
	"ru-informal" => 	"Новое голосовое сообщение (окно: $\{VM_MAILBOX})",
	"ru-formal" => 		"Новое голосовое сообщение (окно: $\{VM_MAILBOX})",
	
	// swedish
	"se-informal" => 	"Nytt röstmeddelande (box: $\{VM_MAILBOX})"
);

/**
 * Voicemail notification e-mail bodies
 * XXX : consolidate in help.inc or something like languages.inc
 */
$vm_emailbody = array(

	// danish
	"da-informal" => 	"Kære $\{VM_NAME},\\n\\n".
						"En besked fra $\{VM_CALLERID} er modtaget på din telefonsvarer. ".
						"En $\{VM_DUR} lang besked blev indtalt til dig den $\{VM_DATE}.\\n\\n".
						"-AskoziaPBX",

	"da-formal" => 		"Kære $\{VM_NAME},\\n\\n".
						"En $\{VM_DUR} lang besked blev indtalt på din telefonsvarer den $\{VM_DATE} ".
						"fra $\{VM_CALLERID}.\\n\\n".
						"-AskoziaPBX",

	// german
	"de-informal" => 	"Hallo $\{VM_NAME},\\n\\n".
						"eine $\{VM_DUR} lange Nachricht wurde von $\{VM_CALLERID} am $\{VM_DATE} hinterlassen.\\n\\n".
						"-AskoziaPBX",

	"de-formal" => 		"Sehr geehrte(r) $\{VM_NAME},\\n\\n".
						"eine $\{VM_DUR} lange Sprachmitteilung wurde von $\{VM_CALLERID} am $\{VM_DATE} für Sie hinterlassen.\\n\\n".
						"-AskoziaPBX",
	
	// english
	"en-informal" => 	"Dear $\{VM_NAME},\\n\\n".
						"A call from $\{VM_CALLERID} just landed in your voicemail.\\n\\n".
						"Here's the $\{VM_DUR} long message that was left for you on $\{VM_DATE}.\\n\\n".
						"-AskoziaPBX",

	"en-formal" => 		"Dear $\{VM_NAME},\\n\\n".
						"A $\{VM_DUR} long voicemail was left for you on $\{VM_DATE} from $\{VM_CALLERID}.\\n\\n".
						"-AskoziaPBX",

	// spanish
	"es-formal" => 		"Estimado $\{VM_NAME},\\n\\n".
						"Usted ha recibido un mensaje de voz desde el número $\{VM_CALLERID}.\\n\\n".
						"La duración del mensaje es $\{VM_DUR} y fue dejado para usted el día $\{VM_DATE}.\\n\\n".
						"-AskoziaPBX",

	// french
	"fr-informal" => 	"Cher $\{VM_NAME},\\n\\n".
						"Un message de $\{VM_CALLERID} t' attend dans ta boite vocale.\\n\\n".
						"Tu trouvera en pièce jointe le message d'une durée de $\{VM_DUR} qui t'a été laissé en date du $\{VM_DATE}.\\n\\n".
						"-AskoziaPBX",

	"fr-formal" => 		"Cher $\{VM_NAME},\\n\\n".
						"Un message de $\{VM_CALLERID} vous attend dans votre boite vocale.\\n\\n".
						"Vous trouverez en pièce jointe le message d'une durée de $\{VM_DUR} qui vous a été laissé en date du $\{VM_DATE}.\\n\\n".
						"-AskoziaPBX",

	// italian
	"it-formal" => 		"Gentile $\{VM_NAME},\\n\\n".
						"e' stato ricevuto un messaggio vocale per Lei dal numero telefonico ".
						"$\{VM_CALLERID}. Il messaggio e' stato ricevuto il giorno $\{VM_DATE} e la ".
						"durata e' $\{VM_DUR}.\\n\\n".
						"-AskoziaPBX",

	"it-informal" =>	"Ciao $\{VM_NAME},\\n\\n".
						"hai un messaggio vocale in voicemail!\\n\\n".
						"Il numero telefonico del chiamante e' $\{VM_CALLERID}.\\n".
						"Data $\{VM_DATE} - durata $\{VM_DUR}.\\n\\n".
						"-AskoziaPBX",

	// dutch
	"nl-informal" => 	"Hallo $\{VM_NAME},\\n\\n".
						"Een oproep van $\{VM_CALLERID} is in je voicemail terecht gekomen.\\n\\n".
						"Hier is het $\{VM_DUR} durende bericht dat voor jou is achtergelaten op $\{VM_DATE}.\\n\\n".
						"-AskoziaPBX",

	"nl-formal" => 		"Beste $\{VM_NAME},\\n\\n".
						"Een oproep van $\{VM_CALLERID} is in uw voicemail terecht gekomen.\\n\\n".
						"Hier is het $\{VM_DUR} durende bericht dat voor u is achtergelaten op $\{VM_DATE}.\\n\\n".
						"-AskoziaPBX",

	// polish
	"pl-informal" => 	"Szanowny $\{VM_NAME},\\n\\n".
						"Wiadomość głosowa od $\{VM_CALLERID} została pozostawiona na ".
						"twojej skrzynce. Wiadomość trwa $\{VM_DUR} i została pozostawiona ".
						"$\{VM_DATE}.\\n\\n".
						"-AskoziaPBX",

	"pl-formal" => 		"Szanowny $\{VM_NAME},\\n\\n".
						"$\{VM_CALLERID} pozostawił trwającą $\{VM_DUR} wiadomość głosową w ".
						"dniu $\{VM_DATE}.\\n\\n".
						"-AskoziaPBX",

	// portuguese, brazil
	"pt-br-informal" => "Caro(a) $\{VM_NAME},\\n\\n".
						"Uma chamada de $\{VM_CALLERID} acabou de cair em sua caixa postal de voz.\\n\\n".
						"Aqui está a mensagem deixada no dia $\{VM_DATE}, que tem $\{VM_DUR} de duração.\\n\\n".
						"-AskoziaPBX",

	"pt-br-formal" => 	"Caro(a) $\{VM_NAME},\\n\\n".
						"Uma mensagem com $\{VM_DUR} chegou para você no dia $\{VM_DATE}, vinda de $\{VM_CALLERID}.\\n\\n".
						"-AskoziaPBX",

	// russian
	"ru-informal" => 	"Уважаемые $\{VM_NAME},\\n\\n".
						"Вам звонил $\{VM_CALLERID} и оставил сообщение в голосовом ящике.\\n\\n".
						"Продолжительность и дата сообщения $\{VM_DUR}, $\{VM_DATE}.\\n\\n".
						"-AskoziaPBX",

	"ru-formal" => 		"Уважаемые $\{VM_NAME},\\n\\n".
						"Сообщение продолжительностью $\{VM_DUR}, от $\{VM_DATE} было оставленно\\n\\n".
						"для Вас от $\{VM_CALLERID}.\\n\\n".
						"-AskoziaPBX",

	// swedish
	"se-informal" => 	"Bäste $\{VM_NAME},\\n\\n".
						"Ett samtal ifrån $\{VM_CALLERID} har precis spelats in på din telesvar.\\n\\n".
						"Meddelandet är $\{VM_DUR} långt och det lämnades $\{VM_DATE}.\\n\\n".
						"-AskoziaPBX"
);

/**
 * Generates voicemail.conf
 * XXX : security issues here with 0000 password on mail-only accounts?
 */
function voicemail_conf_generate() {
	global $config, $vm_emailsubject, $vm_emailbody;
	
	$fd = fopen("/usr/local/etc/asterisk/voicemail.conf", "w");
	if (!$fd) {
		printf("Error: cannot open voicemail.conf in voicemail_conf_generate().\n");
		return 1;
	}
	
	$conf = <<<EOD

[general]
format=wav49
attach=yes
skipms=3000
maxsilence=10
silencethreshold=128
maxlogins=3
minmessage=3
charset=UTF-8

fromstring=AskoziaPBX Voicemail

EOD;

	if ($config['voicemail']['fromaddress']) {
		$conf .= "serveremail=" . $config['voicemail']['fromaddress'] . "\n\n";
	} else if ($config['voicemail']['address']) {
		$conf .= "serveremail=" . $config['voicemail']['address'] . "\n\n";
	} else {
		$conf .= "serveremail=AskoziaPBX\n\n";
	}

	if ($config['voicemail']['maillanguage']) {
		$vm_lang = $config['voicemail']['maillanguage'];
		$conf .= "emailsubject=" . $vm_emailsubject[$vm_lang] . "\n";
		$conf .= "emailbody=" . $vm_emailbody[$vm_lang] . "\n\n";
	} else {
		$conf .= "emailsubject=" . $vm_emailsubject['en-informal'] . "\n";
		$conf .= "emailbody=" . $vm_emailbody['en-informal'] . "\n\n";
	}

	$conf .= <<<EOD

emaildateformat=%Y-%m-%d (%R)

mailcmd=/usr/local/bin/msmtp -C /etc/msmtp.conf -t

[zonemessages]
eastern=America/New_York|'vm-received' Q 'digits/at' IMp
central=America/Chicago|'vm-received' Q 'digits/at' IMp
central24=America/Chicago|'vm-received' q 'digits/at' H N 'hours'
military=Zulu|'vm-received' q 'digits/at' H N 'hours' 'phonetic/z_p'
european=Europe/Copenhagen|'vm-received' a d b 'digits/at' HM

[default]

EOD;

	// internal phones
	$intphones = pbx_get_phones();
	foreach($intphones as $p) {
		if(!$p['voicemailbox'])
			continue;
		// is eliminating VoiceMailMain from the dialplan enough to keep
		// this default password from being a privacy risk?
		$conf .= "{$p['extension']} => 0000,{$p['callerid']},{$p['voicemailbox']},,delete=yes\n";
	}
	
	// external phones
	$extphones = external_get_phones();
	foreach($extphones as $p) {
		if(!$p['voicemailbox'])
			continue;
		$conf .= "{$p['extension']} => 0000,{$p['name']},{$p['voicemailbox']},,delete=yes\n";
	}
		
	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}

/**
 * Generates msmtp.conf for smtp client
 */
function msmtp_conf_generate() {
	global $config;
	
	$fd = fopen("/etc/msmtp.conf", "w");
	if (!$fd) {
		printf("Error: cannot open msmtp.conf in msmtp_conf_generate().\n");
		return 1;
	}
	
	$vmconfig = $config['voicemail'];

	$address = explode("@", $vmconfig['address']);
	$domain = $address[1];
	
	$conf = "account default\n";
	$conf .= "host {$vmconfig['host']}\n";
	
	if ($vmconfig['username']) {
		$conf .= "auth on\n";
		$conf .= "user {$vmconfig['username']}\n";
		$conf .= "password {$vmconfig['password']}\n";
	}
	
	if ($vmconfig['tls']) {
		$conf .= "tls on\n";
		$conf .= "tls_starttls on\n";
		$conf .= "tls_certcheck off\n";
	}
	
	if ($vmconfig['fromaddress']) {
		$conf .= "from {$vmconfig['fromaddress']}\n";
	} else {
		$conf .= "from {$vmconfig['address']}\n";
	}
	
	$conf .= "maildomain $domain\n";
	
	if ($vmconfig['port']) {
		$conf .= "port {$vmconfig['port']}\n";
	}
	
	$conf .= "syslog LOG_MAIL\n";

	fwrite($fd, $conf);
	fclose($fd);
	
	chmod("/etc/msmtp.conf", 0600);

	return 0;
}

/**
 * Reloads app_voicemail to apply newly generated changes
 */
function voicemail_reload() {
	return pbx_exec("module reload app_voicemail.so");
}

?>
