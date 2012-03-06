<?php

/**
 * Display a list of videos ordered by submission
 */
function aai_videos_list(){
	$path = drupal_get_path('module', 'aai_videos');
	drupal_add_css($path.'/css/aai_videos.css');
	module_load_include('rest.inc', 'aai_videos', 'includes/aai_videos');
	
	$output = '';
	
	// Fetch video list from API
	$request = new RestRequest(NULL, 'GET');
	$request->execute();
	
	if($request->getHttpCode() != 200){
		drupal_set_message(t('We had trouble connecting to our video list server. Please try again soon.', 'error'));
	}else{
		$output .= theme('aai_video_list', $request->getResponseBody());
	}
	
	// Add new video link to page
	// Check to make sure today is not a weekend
	if(aai_videos_is_allowed()){
		$output .= drupal_get_form('aai_videos_create_form');
	}
	
	return $output;
}

/**
 * New video link form
 */
function aai_videos_create_form(){
	$form = array();
	$form['create'] = array(
	  '#type' => 'fieldset',
	  '#title' => t('Add a video link'),
	  '#collapsible' => TRUE,
	  '#collapsed' => FALSE,
	);
	$form['create']['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Video Title'),
    '#size' => '60',
    '#maxlength' => '255',
	  '#required' => TRUE
  );
	$form['create']['url'] = array(
    '#type' => 'textfield',
    '#title' => t('Video URL'),
    '#description' => t('We prefer videos on YouTube or Vimeo.'),
    '#size' => '60',
    '#maxlength' => '255',
	  '#required' => TRUE
  );
  $form['create']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add'),
  );
	return $form;
}

/**
 * Validation for video link form
 */
function aai_videos_create_form_validate($form, &$form_state){
	// Check to make sure today is not a weekend
	if(!aai_videos_is_allowed()){
		form_set_error('', t('Links can only be submitted on weekdays.'));
	}
	if(!isValidURL(check_plain($form_state['values']['url']))){
		form_set_error('url', t('The URL you submitted is not valid.'));
	}
}

/**
 * Submit for video link form
 */
function aai_videos_create_form_submit($form, &$form_state){
	
	$query = array(
		'title' => check_plain($form_state['values']['title']),
		'url' => check_plain($form_state['values']['url'])
	);
	
	$request = new RestRequest(NULL, 'POST', $query);
	$request->execute();
	
	switch($request->getHttpCode()){
		case 201:
			drupal_set_message(t('Your video has been added to our list. HURRAY!'));
			break;
		case 204:
			drupal_set_message(t('This video has already been added to our list. BOOOOO! If we were giving points out to people adding videos to this list... you would get NOTHING.'), 'warning');
			break;
		case 212:
			drupal_set_message(t('Something horrible happened. If you see this, it\'s probably the fault of some lame developer named Cyle. Feel free to send him hate mail!'), 'error');
			break;
	}
}

/**
 * View individual video
 *
 * @param $id
 *   The unique video id.
 * @param $op
 *   The oepration being performed.
 */
function aai_videos_operation($id, $op){
	module_load_include('rest.inc', 'aai_videos', 'includes/aai_videos');
	
	if($op == 'view'){
		return aai_videos_view($id);
	}
	
	// If we have a vote up or vote down request
	if(in_array($op, array('voteup', 'votedown'))){
	
		// Check to make sure today is not a weekend
		if(!aai_videos_is_allowed()){
			drupal_set_message(t('You cannot perform this action on a weekend. Why? No one knows.'), 'error');
			drupal_goto();
		}
		
		// Check if use has reached their vote limit for this video.
		// We are also setting a cookie at this time
		if(aai_videos_limit_check($id, 'vote', TRUE)){
			drupal_set_message(t('You have already voted for this video.'), 'error');
			drupal_goto();
		}
		
		switch($op){
			case 'voteup':	
				$query = array('opinion'=>1);
				$message = 'Your + vote has been counted. You must really like that video!';
				break;
			case 'votedown':
				$query = array('opinion'=>-1);
				$message = 'Your - vote has been counted. You must really hate that video!';
				break;
		}
		$request = new RestRequest('/'.$id.'/votes', 'POST', $query);
		$request->execute();	
		switch($request->getHttpCode()){
			case 201:		
				drupal_set_message(t($message));	
				break;
			case 412:
				drupal_set_message(t('Something horrible happened. If you see this, it\'s probably the fault of some lame developer named Cyle. Feel free to send him hate mail!'), 'error');
				break;
		}
		drupal_goto();
	}
	
	drupal_not_found();
	die;
}

/**
 * View individual video
 *
 * @param $id
 *   The unique video id.
 */
function aai_videos_view($id){
	$path = drupal_get_path('module', 'aai_videos');
	drupal_add_css($path.'/css/aai_videos.css');
	module_load_include('rest.inc', 'aai_videos', 'includes/aai_videos');
	$output = '';
	
	$request = new RestRequest('/'.$id, 'GET');
	$request->execute();
	
	switch($request->getHttpCode()){
		case 200:		
			$video = $request->getResponseBody();
			drupal_set_title(htmlspecialchars_decode($video['title']));
			$output .= theme('aai_video', $video);
			break;
		case 404:
			drupal_not_found();
			die;
	}
	
	return $output;
}

/**
 * Output a list of top 10 videos
 *
 * @param $op
 *   Should equal 'vote' or 'view'
 */
function aai_videos_topten($op){
	module_load_include('rest.inc', 'aai_videos', 'includes/aai_videos');
	$output = '';
	// Fetch cache
	$id = 'aai_videos_topten_'.$op;
	$cache = cache_get($id);
	
	// Since we don't want to run cron every 5 minutes, we check expiration here
	if($cache->expire < time()) $cache = NULL;
	
	if($cache){
		// Use cached data if found
		$videos = $cache->data;
	}else{	
		// Fetch new data
		$request = new RestRequest('/top10/'.$op, 'GET');
		$request->execute();
		
		switch($request->getHttpCode()){
			case 200:		
				$videos = $request->getResponseBody();
				// Cache data for 5 minutes
				cache_set($id, $videos, 'cache', strtotime('+5 minutes'));
				break;
			default:
				drupal_set_message(t('Something horrible happened. If you see this, it\'s probably the fault of some lame developer named Cyle. Feel free to send him hate mail!'), 'error');
				die;
		}
	}
	
	if(!empty($videos)) $output .= theme('aai_video_top10', $op, $videos);
	
	return $output;
}

/**
 * Helper function used to validate a URL
 *
 * @param $url
 *   String URL being validated
 */
function isValidURL($url){
	return preg_match('|^http(s)?://[a-z0-9-]+(.[a-z0-9-]+)*(:[0-9]+)?(/.*)?$|i', $url);
}