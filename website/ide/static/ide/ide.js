let logWindow = null;
let game = null;
let editor = null;
let addLogLine_switches_tab = false;
let logTabs = null;
let sidebarTabs = null;

$(function() {
    logWindow = document.getElementById('log');
    logTabs = new TabBar($('#logtabs'), $('#logviews'));
    logTabs.init();
    $('#show_build_output').click(function() { addLogLine_switches_tab = false; });

    sidebarTabs = new TabBar($('#sidebartabs'), $('#sidebar'));
    sidebarTabs.init();
    $('#persistent_memory').on('select', updatePersistentMemory);
    $('#refresh_persistent_memory').click(updatePersistentMemory);
    $('#persistent_data_upload').click(function() { $('#persistent_data_file').click()});
    $('#persistent_data_download').click(function() {  window.location='/api/v1/profile/persistent_data'; });
    $('#persistent_data_file').on('change', uploadPersistentData);
    $('#persistent_data_clear').click(function() {
       if (confirm("really delete the persistent data?"))
       {
           $.ajax({
              url: '/api/v1/profile/persistent_data',
              type: 'DELETE',
              success: updatePersistentMemory
           });
       }
    });

    setupEditor();
    setupToolbar();
    setupShortcuts();
    sidebarTabs.select(0);
    setupPreview();


    $.ajaxSetup({
        beforeSend: function(xhr, settings) {
            if (!csrfSafeMethod(settings.type) && !this.crossDomain) {
                xhr.setRequestHeader("X-CSRFToken", csrftoken);
            }
        }
    });

    pollCompileState();
});

$(window).resize(function() {
    game.vis.Resize();
});

function setupEditor()
{
    editor = ace.edit("editor");
    editor.setTheme("ace/theme/idle_fingers");
    editor.session.setMode("ace/mode/" + editor_mode);
    editor.setShowPrintMargin(false);
    let textarea = $('#code').hide();
    editor.getSession().setValue(textarea.val());
    $("#snake_edit_form").submit(function(event) {
        textarea.val(editor.getSession().getValue());
    });

    editor.on("input", function() {
        $('#bt_save').prop("disabled", editor.session.getUndoManager().isClean());
        $('#bt_save_as').prop("disabled", editor.session.getUndoManager().isClean());
    });
}

function disablePreview()
{
    if (!window.location.hash.includes('no_preview'))
    {
        window.location.href = window.location.href + '#no_preview';
    }
    game.parseOnlyLogMessages = true;
    sidebarTabs.select(1);
    $('#bt_preview').hide();
    $('#bt_disable_preview').hide();
    $('#bt_enable_preview').show();
}

function enablePreview()
{
    window.location.hash = '';
    window.location.reload();
}

function setupPreview()
{
    game = new Game(assets, strategy, document.getElementById('preview'), function() {
        game.SetViewerKey(viewer_key);
        game.AddLogHandler(addLogLine);
        game.Run();
        game.vis.FollowName(snake_follow_name, true);
    });
    if (window.location.hash.includes('no_preview'))
    {
        disablePreview();
    }
}

function setupToolbar()
{
    $('#bt_run').click(function() {
        save('run', null);
    });

    $('#bt_restart').click(function() {
        $.post('/snake/restart', '', function(data) {
            // todo show data.message
            console.log(data.message)
        });
    });

    $('#bt_stop').click(function() {
        $.post('/snake/disable', '', function(data) {
            // todo show data.message
            console.log(data.message)
        });
    });

    $('#bt_save').click(function() {
       save('save', snake_title);
    });

    $('#bt_save_as').click(function() {
        showModal($('#safe_as_dialog'), function() {
            save('save', $('#save_as_title').val());
        });
        $('#save_as_title').val(snake_title).focus().select();
    });

    $('#bt_load').click(function() {
        $.get("/snake/list", function (data, status) {
            if (status!='success') {
                console.log('loading version list failed: ' + status);
                return;
            }

            if (!data.versions.length) {
                console.log('got empty version list from server');
                return;
            }

            showLoadDialog(data);
        });
    });

    $('#bt_disable_preview').click(disablePreview);
    $('#bt_enable_preview').click(enablePreview);

    $('#sel_programming_language').change(function() {
        window.location.href = '/snake/edit/lang/' + $('#sel_programming_language').val();
    });
}

function setupShortcuts()
{
    $(window).bind('keydown', function(event) {
        if (!(event.ctrlKey || event.metaKey)) {
            return;
        }
        switch (String.fromCharCode(event.which).toLowerCase()) {
            case 's':
                if (event.shiftKey) {
                    $('#bt_save_as').click();
                } else {
                    $('#bt_save').click();
                }
                event.preventDefault();
                break;
            case 'o':
                $('#bt_load').click();
                event.preventDefault();
                break;
            case 'r':
                $('#bt_run').click();
                event.preventDefault();
                break;
        }
    });
}

function pollCompileState()
{
    $.ajax({
        url: '/api/v1/compile_state',
        dataType: 'json',
        success: updateCompileState,
        error: function()
        {
            window.setTimeout(pollCompileState, 10000);
        }
    });
}

function updateCompileState(data)
{
    let state = data.compile_state;

    if (state == "not_compiled")
    {
        $("#build_log").append($('<div/>').addClass('info').text("Build queue length: " + data.build_queue_size));
        window.setTimeout(pollCompileState, 2000);
        return;
    }
    else
    {
        addLogLine_switches_tab = false;
        let c = $("#build_log").empty();
        if (data.build_log)
        {
            data.build_log.forEach(function(item) {
                if (item.i) {
                    c.append($('<div/>').addClass('info').text(item.i));
                }
                if (item.e) {
                    c.append($('<div/>').addClass('err').text(item.e));
                }
                if (item.o) {
                    c.append($('<div/>').addClass('std').text(item.o));
                }
            });
        }
        logTabs.select(0);
    }

    if (state == "successful") {
        window.setTimeout(function() { addLogLine_switches_tab = true; }, 5000);
        $.growl.notice({ message: "&#x2714; Compilation successful" });
    } else if (state == "failed") {
        $.growl.error({ message: "&#x26A1; Compilation failed" });
    } else if (state == "crashed") {
        $.growl.error({ message: "&#x1F480; Crashed on startup" });
    }
}

function showModal(el, ok_func)
{
    let dialog = $(el);
    let blocker = dialog.parents('.modal');
    let bt_ok = dialog.find('.bt_ok');
    let bt_cancel = dialog.find('.bt_cancel');

    blocker.show();
    bt_ok.off().click(function() { ok_func(); hideModal(dialog); });
    bt_cancel.off().click(function() { hideModal(dialog); });

    $(document).off('keydown.modal').on('keydown.modal', function(event) {
        if (event.which === 13) { bt_ok.click(); }
        if (event.which === 27) { bt_cancel.click(); }
    });
}

function hideModal(el)
{
    $(document).off('keydown.modal');
    let blocker = $(el).parents('.modal');
    blocker.find('.bt_ok').off("modal.ok");
    blocker.find('.bt_cancel').off("modal.cancel");
    blocker.hide();
}

function csrfSafeMethod(method) {
    return (/^(GET|HEAD|OPTIONS|TRACE)$/.test(method));
}

function save(action, title)
{
    let json_req = {
        'action': action,
        'code': editor.getSession().getValue(),
        'comment': title,
        'parent': snake_id,
        'programming_language': programming_language
    };

    $.post('/snake/edit/save', JSON.stringify(json_req), function(data) {
        snake_id = data.snake_id;
        snake_title = data.comment;
        game.vis.FollowName(snake_follow_name, true);

        let logline = 'saved code as version #' + data.version;
        if (data.comment) { logline += "(\"" + data.comment + "\")"; }

        $.growl.notice({ message: logline });

        editor.session.getUndoManager().markClean();

        if (action == 'run') {
            addLogLine_switches_tab = false;
            logTabs.select(0);
            let info = $('<div/>').addClass('info').text(logline + ", waiting for build output");
            $("#build_log").empty().append(info);
            pollCompileState();
        }

    });
}

function showLoadDialog(data)
{
    let list = $('#load_dialog .list');
    let selected_version = 'latest';

    let bt_ok = $('#load_dialog .bt_ok');
    bt_ok.prop("disabled", true);
    list.empty();

    $.each(data.versions, function(i, version) {
        let item = $('<div><div>'+version.version+'</div><div>'+version.date+'</div><div>'+version.programming_language+'</div><div>'+version.title+'</div></div>');
        item.click(function() {
            list.children('div').removeClass('selected');
            item.addClass('selected');
            selected_version = version.id;
            $('#load_dialog .bt_ok').prop("disabled", false);
        });
        item.dblclick(function() {
            selected_version = version.id;
            bt_ok.click();
        });
        list.append(item);
    });

    showModal($('#load_dialog'), function() {
        window.location.href = '/snake/edit/' + selected_version;
    });
}

function addLogLine(frame, msg)
{
    if (logWindow==null) { return; }

    let auto_scroll = (logWindow.scrollTop > (logWindow.scrollHeight - logWindow.clientHeight - 10));
    while (logWindow.childNodes.length > 100)
    {
        logWindow.removeChild(logWindow.childNodes[0]);
    }

    let div = document.createElement('div');
    let frameDiv = document.createElement('div');
    if (frame) {
        frameDiv.appendChild(document.createTextNode("Frame " + frame + ":"));
    }
    div.appendChild(frameDiv);
    let msgDiv = document.createElement('pre');
    msgDiv.appendChild(document.createTextNode(msg));
    div.appendChild(msgDiv);
    logWindow.appendChild(div);

    if (auto_scroll)
    {
        logWindow.scrollTop = logWindow.scrollHeight - logWindow.clientHeight;
    }

    if (addLogLine_switches_tab)
    {
        logTabs.select(1);
    }
}

function updatePersistentMemory()
{
    $('#hexdump').text('');
    var req = new XMLHttpRequest();
    req.open('GET', '/api/v1/profile/persistent_data', true);
    req.responseType = "arraybuffer";
    req.onload = function(oEvent) {
        let byteArray = new Uint8Array(req.response)
        $('#hexdump').text(hexy(byteArray));
    };
    req.send();
}

function uploadPersistentData(e)
{
    let file = e.target.files[0];
    $.ajax({
        type: 'POST',
        url: '/api/v1/profile/persistent_data',
        data: file,
        processData: false,
        contentType: false,
        success: updatePersistentMemory
    });
}

window.onbeforeunload = function (e) {
    if (editor.session.getUndoManager().isClean()) return;

    var confirmationMessage = "You have edited your snake, but you didn't save it yet.";

    e.returnValue = confirmationMessage;
    return confirmationMessage;
};
