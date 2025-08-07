import random
import uuid
from django.db import models
from django.utils.timezone import now
from django.contrib.auth.models import User


def get_user_profile(user):
    profile, _ = UserProfile.objects.get_or_create(user=user)
    return profile


class ProgrammingLanguage(models.Model):
    class Meta:
        get_latest_by = "created"
        unique_together = ('slug', 'readable_name')

    # A short version of the Language name that can be used in file names, URLs, etc.
    slug = models.CharField(max_length=16)

    # The official, full name of the Language
    readable_name = models.CharField(max_length=256)

    # The file extension used for source files, without the separating '.'
    file_extension = models.CharField(max_length=8)

    # Mode name for the ACE editor
    editor_mode = models.CharField(max_length=32)

    def __str__(self):
        return f"[{self.slug}] {self.readable_name}"

class SnakeVersion(models.Model):
    class Meta:
        get_latest_by = "created"
        unique_together = ('user', 'version')

    COMPILE_STATE_CHOICES = (
            ('not_compiled', 'Not compiled yet'),
            ('successful',   'Compiled successfully'),
            ('failed',       'Compilation failed'),
            ('crashed',      'Compilation successful, but init failed'),
            )

    user = models.ForeignKey(User, on_delete=models.CASCADE)
    parent = models.ForeignKey("SnakeVersion", blank=True, null=True, on_delete=models.SET_NULL)
    created = models.DateTimeField(default=now, blank=True)
    programming_language = models.ForeignKey("ProgrammingLanguage", blank=True, on_delete=models.PROTECT)
    code = models.TextField()
    comment = models.CharField(max_length=1000, blank=True, null=True)
    version = models.IntegerField()
    server_error_message = models.TextField(blank=True, null=True)
    compile_state = models.CharField(default='not_compiled', max_length=12, choices=COMPILE_STATE_CHOICES)
    build_log = models.TextField(null=True)

    def create_new_if_changed(self, code, comment, programming_language):
        if self.code == code and self.comment == comment:
            return self
        else:
            new_version = SnakeVersion(user=self.user, parent=self, code=code, comment=comment, programming_language=programming_language)
            new_version.save()
            return new_version

    def save(self, *args, **kwargs):
        if not self.version:
            self.version = self.get_max_version_number() + 1
        super(SnakeVersion, self).save(*args, **kwargs)

    def get_max_version_number(self):
        return SnakeVersion.objects.filter(user=self.user).aggregate(models.Max('version'))['version__max'] or 0

    def activate(self):
        UserProfile.objects.update_or_create(user=self.user, defaults={'active_snake': self})

    @staticmethod
    def get_latest_for_user(user, programming_language=None):
        filter_args = {"user": user}
        if programming_language != None:
            filter_args["programming_language"] = programming_language

        return SnakeVersion.objects.filter(**filter_args).latest('created')

    def __str__(self):
        return "Snake " + str(self.version) + " by " + self.user.username

    objects = models.Manager()


class UserProfile(models.Model):
    user = models.OneToOneField(User, on_delete=models.CASCADE)
    viewer_key = models.BigIntegerField(unique=True)
    active_snake = models.ForeignKey(SnakeVersion, null=True, blank=True, on_delete=models.SET_NULL)
    persistent_data = models.BinaryField(max_length=2**20, null=True, blank=True)

    def save(self, *args, **kwargs):
        if not self.viewer_key:
            self.viewer_key = random.getrandbits(63)
        super(UserProfile, self).save(*args, **kwargs)


class SnakeGame(models.Model):
    snake_version = models.ForeignKey(SnakeVersion, on_delete=models.CASCADE)
    user = models.ForeignKey(User, null=True, on_delete=models.SET_NULL)
    start_date = models.DateTimeField(blank=True, null=True)
    end_date = models.DateTimeField(blank=True, null=True)
    start_frame = models.IntegerField(blank=True, null=True)
    end_frame = models.IntegerField(blank=True, null=True)
    killer = models.ForeignKey(User, null=True, blank=True, on_delete=models.SET_NULL, related_name="games_won")
    maximum_mass = models.FloatField(blank=True, null=True)
    final_mass = models.FloatField(blank=True, null=True)
    natural_food_consumed = models.FloatField(blank=True, null=True)
    carrison_food_consumed = models.FloatField(blank=True, null=True)
    hunted_food_consumed = models.FloatField(blank=True, null=True)

    def __str__(self):
        return self.user.username + "@" + str(self.start_date)


class ServerCommand(models.Model):
    COMMAND_CHOICES = (('kill', 'kill'),)
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    dt_created = models.DateTimeField(auto_now_add=True, blank=True)
    dt_processed = models.DateTimeField(null=True, blank=True, editable=False)
    command = models.CharField(max_length=255, choices=COMMAND_CHOICES)
    result = models.NullBooleanField(editable=False)
    result_msg = models.TextField(blank=True, null=True, editable=False)


class LiveStats(models.Model):
    last_update = models.DateTimeField()
    fps = models.FloatField()
    current_frame = models.IntegerField()
    running_bots = models.IntegerField()
    start_queue_len = models.IntegerField()
    stop_queue_len = models.IntegerField()
    living_mass = models.FloatField()
    dead_mass = models.FloatField()


def create_key():
    return str(uuid.uuid4())


class ApiKey(models.Model):
    MAX_KEYS_PER_USER = 20
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    key = models.CharField(max_length=100, default=create_key)
    comment = models.CharField(max_length=255, null=True, blank=True)
