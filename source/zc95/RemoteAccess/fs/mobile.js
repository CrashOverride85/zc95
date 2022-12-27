
(function($) {
    
    var selectedSSID, frequency, needsUsername = true, needsPassword = true;

    /*
     * Wifi network scan results are retrieved as JSON
     * through an AJAX call, and look like this:
     *
     *  [{
     *     'ssid': 'standard psk wifi network',
     *     'rssi': -50,
     *     'needsUsername': false,
     *     'needsPassword': true
     *   },{
     *     'ssid': 'an open wifi network',
     *     'rssi': -10,
     *     'needsUsername': false,
     *     'needsPassword': false
     *   }]
     *
     */
    var scanresults;
    
    $(document).on('pagecreate', '#wifinetworks', function()
    {
        /* Fetch scan results */
        $.ajax('/wifi/scanresults.ssi', {dataType: 'json', success: function(scanresults) {
            /* Generate the scan results page */
            
            var html = ''; 
            for (var i=0; i < scanresults.length; i++)
            {
                var ssid = scanresults[i].ssid;
                var rssi = scanresults[i].rssi;                
                var icon = "/icons/network-wireless-connected-";
                
                if (rssi > -50)
                    icon += "100.png";
                else if (rssi > -60)
                    icon += "75.png";
                else if (rssi > -70)
                    icon += "50.png";
                else if (rssi > -80)
                    icon += "25.png";
                else
                    icon += "00.png";
                
                html += "<li><a href='#' class='network' id='net-"+i+"'><img src='"+icon+"' class='ui-li-icon'>"+ssid+"</a></li>";
            }
            
            /* Refresh the listview */
            $('#networklist').empty().append(html).listview().listview('refresh');
    
            /* If network is selected display authorization page */
            $('.network').on('click', function() {
                var net = scanresults[this.id.substr(4)];
    
                selectedSSID = net.ssid;
                needsUsername = net.needsUsername;
                needsPassword = net.needsPassword;
                $('#ssid').text(selectedSSID);
                $('#username').val('');
                $('#password').val('');            
                if (needsUsername)
                {
                    $('#usernameblock').show();
                }
                else
                {
                    $('#usernameblock').hide();
                }
                if (needsPassword)
                {
                    $('#passwordblock').show();
                }
                else
                {
                    $('#passwordblock').hide();
                }
                
                $.mobile.changePage($('#wifilogin'));
            });
        }});
    });
    
    $(document).on('pageshow', '#wifilogin', function()
    {
        /* Set focus to the appropiate input field, depending on information needed */
        if (needsUsername)
        {
            $('#username').focus();
        }
        else if (needsPassword)
        {
            $('#password').focus();
        }
        else
        {
            $('#connectbutton').focus();
        }
    });
    
    $(document).on('click', '#connectbutton', function()
    {
        var username = $('#username').val();
        var password = $('#password').val();
        //var jsondata = JSON.stringify({ssid: selectedSSID, username: username, password: password});
        
        if (needsPassword && password.length < 8)
        {
            alert("WPA PSK password needs to be at least 8 characters");
            return;
        }

        //$.ajax('/wifi/setnetwork', {type: 'POST', data: jsondata, contentType: 'application/json'});
        $.ajax('/wifi/setnetwork', {type: 'GET', data: {ssid: selectedSSID, username: username, password: password}});
        $('#networkstatus').html('');
        $.mobile.changePage($('#wificonnecting'));        
    });

})(jQuery);
