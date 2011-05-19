						</td>
					</tr>
				</table>
			</td>
	 	</tr>
		<tr align="center" valign="top" class="webgui_bg"> 
		<td colspan="2" class="cpline"><?=gettext("Askozia&reg;PBX &copy; 2007-2011 IKT. All rights reserved.");?>&nbsp; [<a target="_blank" href="http://www.askozia.com/pbx-license" class="tblnk"><?=gettext("view license");?></a>]
		</td>
	</tr>
	</table>
	</div></div>
</body>
</html><?

if (file_exists($d_passworddirty_path)) {
	system_update_shell_password();
	unlink($d_passworddirty_path);
}

?>
