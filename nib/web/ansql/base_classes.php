<?php
/**
 * Copyright (C) 2014 Null Team
 *
 * This software is distributed under multiple licenses;
 * see the COPYING file in the main directory for licensing
 * information for this specific distribution.
 *
 * This use of this software may be subject to additional restrictions.
 * See the LEGAL file in the main directory for details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */ 

class Response extends GenericStatus
{
	// optional generic $info, in case you need to return a specific object/value
	private $info;

	public function __construct($status=true, $p1=NULL, $p2=NULL)
	{
		parent::__construct($status, $p1, $p2);
		if ($status)
			$this->info = $p1;
	}

	public function getResult()
	{
		return $this->info;
	}
}

class GenericStatus
{
	private $status = true;
	private $error_message;
	private $error_code;

	public function __construct($status=true, $p1=NULL, $p2=NULL)
	{
		$this->status = $status;

		if (!$this->status) {
			if (is_numeric($p1)) {
				$this->code = $p1;
				$this->error_message = $p2;
			} else {
				$this->error_message = $p2;
				$this->code = $p1;
			}
		}
	}

	public function status()
	{
		return $this->status;
	}

	public function setError($p1=NULL,$p2=NULL)
	{
		$this->status = false;
		if (is_numeric($p1)) {
			$this->code = $p1;
			$this->error_message = $p2;
		} else {
			$this->error_message = $p1;
			$this->code = $p2;
		}
	}

	/**
	 *  Set error message. If error message is already set concatenate it.
	 * @param $message String
	 */
	private function setErrorMessage($message)
	{
		if (isset($this->error_message) && strlen($this->error_message))
			$this->error_message .= "\n".$message;
		else
			$this->error_message = $message;
	}

	/**
	 * Set error code. If error code is already set, keep the initial code. We assume that when initial error was triggered the normal flow was already interrupted and this next are errors for actions trying to recover
	 * @param $code String
	 */
	private function setErrorCode($code)
	{
		if (isset($this->code) && strlen($this->code))
			return;

		$this->code = $code;
	}

	public function getError()
	{
		return $this->error_message;
	}

	public function getErrorCode()
	{
		return $this->error_code;
	}
}

?>
