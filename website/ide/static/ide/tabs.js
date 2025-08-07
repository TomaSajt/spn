"use strict";

const TabBar = function(tab_bar, tab_container)
{
    this.tab_bar = tab_bar;
    this.tab_container = tab_container;
};

TabBar.prototype.init = function()
{
    let self = this;
    this.tab_bar.children().each(function(i, el) {
        let tab_id = i;
        $(el).click(function() {
            self.select(tab_id);
        })
    });
};

TabBar.prototype.select = function(active_tab_index)
{
    this.tab_bar.children().removeClass('active');
    this.tab_container.children().removeClass('active');
    this.tab_bar.children().eq(active_tab_index).addClass('active');

    let view = this.tab_container.children().eq(active_tab_index);
    view.addClass('active');
    view[0].dispatchEvent(new Event('select'))
};
