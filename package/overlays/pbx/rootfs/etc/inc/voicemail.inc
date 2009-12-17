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


function voicemail_get_configuration() {
	return $GLOBALS['config']['voicemail'];
}

function voicemail_save_configuration($newconfig) {
	$GLOBALS['config']['voicemail'] = $newconfig;
	write_config();
}

/**
 * Voicemail notification e-mail subject lines
 * XXX : consolidate in help.inc or something like languages.inc
 */
$vm_emailsubject = array(

	// bulgarian
	"bg-informal" =>	"Ново Гласово съобщение (кутия: $\{VM_MAILBOX})",
	"bg-formal"   =>	"Ново Гласово съобщение (кутия: $\{VM_MAILBOX})",

	// czech
	"cs-informal" => 	"Máte zprávu v hlasové schránce (schránka: $\{VM_MAILBOX})",
	"cs-formal" => 		"Vaše hlasová schárnka obsahuje novou zprávu  (schránka: $\{VM_MAILBOX})",

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

	// finnish
	"fi-informal" =>	"Uusi puhesanoma (postilokero: $\{VM_MAILBOX})",
	"fi-formal" =>		"Uusi puhesanoma (postilokero: $\{VM_MAILBOX})",

	// french
	"fr-informal" => 	"tu as reçu un nouveau message dans ta boite vocale",
	"fr-formal" => 		"vous avez reçu un nouveau message dans votre boite vocale",

	// greek
	"gr-informal" => 	"Νέο φωνητικό μήνυμα (θυρίδα: $\{VM_MAILBOX})",
	"gr-formal" => 		"Νέο φωνητικό μήνυμα (θυρίδα: $\{VM_MAILBOX})",

	// italian
	"it-formal" => 		"Nuovo messaggio voicemail (box: $\{VM_MAILBOX})",
	"it-informal" =>	"Nuovo messaggio voicemail per te (box: $\{VM_MAILBOX})",

	// japanese
	"jp-formal" => 		"内線番号:$\{VM_MAILBOX}に、新しいメッセージがあります。",

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
	"se-informal" => 	"Nytt röstmeddelande (box: $\{VM_MAILBOX})",

	// turkish
	"tr-informal" =>	"$\{VM_DATE} de $\{VM_MAILBOX}'e bırakılan Sesli Mesaj",
	"tr-formal" =>		"$\{VM_DATE} de $\{VM_MAILBOX}'e bırakılan Sesli Mesaj"
);

/**
 * Voicemail notification e-mail bodies
 */
$product_name = system_get_product_name();
$vm_emailbody = array(

	// bulgarian
	"bg-informal" =>	"Уважаеми $\{VM_NAME},\\n\\n".
						"Гласово съобщение от $\{VM_CALLERID} се получи във вашата гласова поща. ".
						"Продължителноста му е $\{VM_DUR} и е оставено на $\{VM_DATE}.\\n\\n".
						"-$product_name",

	"bg-formal" =>		"Уважаеми $\{VM_NAME},\\n\\n".
						"Гласово съобщение с продължителност $\{VM_DUR} беше оставено за вас ".
						"на $\{VM_DATE} от $\{VM_CALLERID}.\\n\\n".
						"-$product_name",

	// czech
	"cs-informal" => 	"Vážený/vázená $\{VM_NAME},\\n\\n".
						"ve vaší schránce byl zanechán vzkaz. ". 
						"Zpráva byla zaznamenána $\{VM_DATE} a je $\{VM_DUR} dlouhá.\\n\\n".
						"-$product_name",

	"cs-formal" => 		"Vážený/vázená $\{VM_NAME},\\n\\n".
						"vaše hlasová schárnka obsahuje novou zprávu. ".
						"Zpráva byla zanechána dne $\{VM_DATE}. Delka zprávy: $\{VM_DUR}.\\n\\n".
						"-$product_name",

	// danish
	"da-informal" => 	"Kære $\{VM_NAME},\\n\\n".
						"En besked fra $\{VM_CALLERID} er modtaget på din telefonsvarer. ".
						"En $\{VM_DUR} lang besked blev indtalt til dig den $\{VM_DATE}.\\n\\n".
						"-$product_name",

	"da-formal" => 		"Kære $\{VM_NAME},\\n\\n".
						"En $\{VM_DUR} lang besked blev indtalt på din telefonsvarer den $\{VM_DATE} ".
						"fra $\{VM_CALLERID}.\\n\\n".
						"-$product_name",

	// german
	"de-informal" => 	"Hallo $\{VM_NAME},\\n\\n".
						"eine $\{VM_DUR} lange Nachricht wurde von $\{VM_CALLERID} am $\{VM_DATE} hinterlassen.\\n\\n".
						"-$product_name",

	"de-formal" => 		"Sehr geehrte(r) $\{VM_NAME},\\n\\n".
						"eine $\{VM_DUR} lange Sprachmitteilung wurde von $\{VM_CALLERID} am $\{VM_DATE} für Sie hinterlassen.\\n\\n".
						"-$product_name",
	
	// english
	"en-informal" => 	"Dear $\{VM_NAME},\\n\\n".
						"A call from $\{VM_CALLERID} just landed in your voicemail.\\n\\n".
						"Here's the $\{VM_DUR} long message that was left for you on $\{VM_DATE}.\\n\\n".
						"-$product_name",

	"en-formal" => 		"Dear $\{VM_NAME},\\n\\n".
						"A $\{VM_DUR} long voicemail was left for you on $\{VM_DATE} from $\{VM_CALLERID}.\\n\\n".
						"-$product_name",

	// spanish
	"es-formal" => 		"Estimado $\{VM_NAME},\\n\\n".
						"Usted ha recibido un mensaje de voz desde el número $\{VM_CALLERID}.\\n\\n".
						"La duración del mensaje es $\{VM_DUR} y fue dejado para usted el día $\{VM_DATE}.\\n\\n".
						"-$product_name",

	// finnish
	"fi-informal" => 	"Hei $\{VM_NAME},\\n\\n".
						"Puhelu $\{VM_CALLERID}:lta on saapunut sinun vastaajaan.\\n\\n".
						"Viesti on $\{VM_DUR} pitkä ja on jätetty vastaajalle $\{VM_DATE}.\\n\\n".
						"-$product_name",

	"fi-formal" => 		"Hyvä herra/rouva $\{VM_NAME},\\n\\n".
						"$\{VM_CALLERID} on jättänyt sinulle $\{VM_DUR} pitkän puheviestin $\{VM_DATE}.\\n\\n".
						"-$product_name",

	// french
	"fr-informal" => 	"Cher $\{VM_NAME},\\n\\n".
						"Un message de $\{VM_CALLERID} t' attend dans ta boite vocale.\\n\\n".
						"Tu trouvera en pièce jointe le message d'une durée de $\{VM_DUR} qui t'a été laissé en date du $\{VM_DATE}.\\n\\n".
						"-$product_name",

	"fr-formal" => 		"Cher $\{VM_NAME},\\n\\n".
						"Un message de $\{VM_CALLERID} vous attend dans votre boite vocale.\\n\\n".
						"Vous trouverez en pièce jointe le message d'une durée de $\{VM_DUR} qui vous a été laissé en date du $\{VM_DATE}.\\n\\n".
						"-$product_name",

	// greek
	"gr-formal" => 		"Αγαπητέ/ή $\{VM_NAME},\\n\\n".
						"Έχετε ένα νέο ηχητικό μήνυμα διάρκειας $\{VM_DUR}. ".
						"Το μήνυμα ηχογραφήθηκε στις $\{VM_DATE} από το $\{VM_CALLERID}.\\n\\n".
						"-$product_name",

	"gr-informal" => 	"Αγαπητέ/ή $\{VM_NAME},\\n\\n".
						"Μια κλήση από το $\{VM_CALLERID} έφτασε στη θυρίδα σας στις $\{VM_DATE}. ".
						"Ακολουθεί το μήνυμα με διάρκεια $\{VM_DUR}.\\n\\n".
						"-$product_name",

	// italian
	"it-formal" => 		"Gentile $\{VM_NAME},\\n\\n".
						"e' stato ricevuto un messaggio vocale per Lei dal numero telefonico ".
						"$\{VM_CALLERID}. Il messaggio e' stato ricevuto il giorno $\{VM_DATE} e la ".
						"durata e' $\{VM_DUR}.\\n\\n".
						"-$product_name",

	"it-informal" =>	"Ciao $\{VM_NAME},\\n\\n".
						"hai un messaggio vocale in voicemail!\\n\\n".
						"Il numero telefonico del chiamante e' $\{VM_CALLERID}.\\n".
						"Data $\{VM_DATE} - durata $\{VM_DUR}.\\n\\n".
						"-$product_name",

	// japanese
	"jp-formal" =>		"$\{VM_NAME} 様,\\n\\n".
						"あなた宛に、$\{VM_DATE}に$\{VM_CALLERID}からお電話があり、メッセージをお預かり致しました。(メッセージ録音時間:$\{VM_DUR})\\n\\n".
						"-$product_name",

	// dutch
	"nl-informal" => 	"Hallo $\{VM_NAME},\\n\\n".
						"Een oproep van $\{VM_CALLERID} is in je voicemail terecht gekomen.\\n\\n".
						"Hier is het $\{VM_DUR} durende bericht dat voor jou is achtergelaten op $\{VM_DATE}.\\n\\n".
						"-$product_name",

	"nl-formal" => 		"Beste $\{VM_NAME},\\n\\n".
						"Een oproep van $\{VM_CALLERID} is in uw voicemail terecht gekomen.\\n\\n".
						"Hier is het $\{VM_DUR} durende bericht dat voor u is achtergelaten op $\{VM_DATE}.\\n\\n".
						"-$product_name",

	// polish
	"pl-informal" => 	"Szanowny $\{VM_NAME},\\n\\n".
						"Wiadomość głosowa od $\{VM_CALLERID} została pozostawiona na ".
						"twojej skrzynce. Wiadomość trwa $\{VM_DUR} i została pozostawiona ".
						"$\{VM_DATE}.\\n\\n".
						"-$product_name",

	"pl-formal" => 		"Szanowny $\{VM_NAME},\\n\\n".
						"$\{VM_CALLERID} pozostawił trwającą $\{VM_DUR} wiadomość głosową w ".
						"dniu $\{VM_DATE}.\\n\\n".
						"-$product_name",

	// portuguese, brazil
	"pt-br-informal" => "Caro(a) $\{VM_NAME},\\n\\n".
						"Uma chamada de $\{VM_CALLERID} acabou de cair em sua caixa postal de voz.\\n\\n".
						"Aqui está a mensagem deixada no dia $\{VM_DATE}, que tem $\{VM_DUR} de duração.\\n\\n".
						"-$product_name",

	"pt-br-formal" => 	"Caro(a) $\{VM_NAME},\\n\\n".
						"Uma mensagem com $\{VM_DUR} chegou para você no dia $\{VM_DATE}, vinda de $\{VM_CALLERID}.\\n\\n".
						"-$product_name",

	// russian
	"ru-informal" => 	"Уважаемые $\{VM_NAME},\\n\\n".
						"Вам звонил $\{VM_CALLERID} и оставил сообщение в голосовом ящике.\\n\\n".
						"Продолжительность и дата сообщения $\{VM_DUR}, $\{VM_DATE}.\\n\\n".
						"-$product_name",

	"ru-formal" => 		"Уважаемые $\{VM_NAME},\\n\\n".
						"Сообщение продолжительностью $\{VM_DUR}, от $\{VM_DATE} было оставленно\\n\\n".
						"для Вас от $\{VM_CALLERID}.\\n\\n".
						"-$product_name",

	// swedish
	"se-informal" => 	"Bäste $\{VM_NAME},\\n\\n".
						"Ett samtal ifrån $\{VM_CALLERID} har precis spelats in på din telesvar.\\n\\n".
						"Meddelandet är $\{VM_DUR} långt och det lämnades $\{VM_DATE}.\\n\\n".
						"-$product_name",

	// turkish
	"tr-informal" =>	"Merhaba ($\{VM_MAILBOX}) $\{VM_NAME},\\n\\n".
						"seni arayan ve ulaşamayan $\{VM_CALLERID} $\{VM_DUR} uzunluğundaki ".
						"sesli mesajı $\{VM_DATE} tarihinde bırakmıştır.\\n\\n".
						"-$product_name",

	"tr-formal" =>		"Merhaba ($\{VM_MAILBOX}) $\{VM_NAME},\\n\\n".
						"sizi arayan ve ulaşamayan $\{VM_CALLERID} $\{VM_DUR} uzunluğundaki ".
						"sesli mesajı $\{VM_DATE} tarihinde bırakmıştır.\\n\\n".
						"-$product_name"
);

/**
 * Generates voicemail.conf
 * XXX : security issues here with 0000 password on mail-only accounts?
 */
function voicemail_conf_generate($return=false) {
	global $vm_emailsubject, $vm_emailbody;

	$voicemail_config = voicemail_get_configuration();
	$email_config = notifications_get_email_configuration();

	$product_name = system_get_product_name();
	$conf = <<<EOD

[general]
format=wav49
attach=yes
skipms=3000
maxsilence=4
silencethreshold=128
maxlogins=3
minsecs=5
charset=UTF-8

fromstring=$product_name Voicemail

EOD;

	if ($voicemail_config['fromaddress']) {
		$conf .= "serveremail=" . $voicemail_config['fromaddress'] . "\n";
	} else {
		$conf .= "serveremail=" . $email_config['address'] . "\n";
	}

	if (isset($voicemail_config['user_defined'])) {

		$tokens = array("VM_MAILBOX", "VM_NAME", "VM_DURATION", "VM_CALLERID","VM_DATE");
		$replacetokens = array("$" . "{VM_MAILBOX}", "$" . "{VM_NAME}", "$" ."{VM_DUR}", "$" . "{VM_CALLERID}", "$" . "{VM_DATE}");

		$subject = str_replace($tokens, $replacetokens, base64_decode($voicemail_config['user_subject']));
		$body = preg_replace('/\x0A/', "\\n", str_replace($tokens, $replacetokens, base64_decode($voicemail_config['user_body'])));

		$conf .= "emailsubject=" . $subject . "\n";
		$conf .= "emailbody=" . $body . "\n\n";
	} else if ($voicemail_config['maillanguage']) {
		$vm_lang = $voicemail_config['maillanguage'];
		$conf .= "emailsubject=" . $vm_emailsubject[$vm_lang] . "\n";
		$conf .= "emailbody=" . $vm_emailbody[$vm_lang] . "\n\n";
	} else {
		$conf .= "emailsubject=" . $vm_emailsubject['en-informal'] . "\n";
		$conf .= "emailbody=" . $vm_emailbody['en-informal'] . "\n\n";
	}

	$conf .= <<<EOD

emaildateformat=%Y-%m-%d (%R)

mailcmd=/usr/bin/msmtp -C /etc/msmtp.conf -t

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
		if ($p['vmtoemail'] != "enabled") {
			continue;
		}
		// is eliminating VoiceMailMain from the dialplan enough to keep
		// this default password from being a privacy risk?
		$conf .= "{$p['extension']} => 0000,{$p['callerid']},{$p['vmtoemailaddress']},,delete=yes\n";
	}

	// external phones
	$extphones = external_get_phones();
	foreach($extphones as $p) {
		if ($p['vmtoemail'] != "enabled") {
			continue;
		}
		$conf .= "{$p['extension']} => 0000,{$p['callerid']},{$p['vmtoemailaddress']},,delete=yes\n";
	}

	return ($return) ? $conf : util_file_write("/etc/asterisk/voicemail.conf", $conf);
}

?>
